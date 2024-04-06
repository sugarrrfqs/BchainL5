#include <iostream>
#include "mpi.h"
#include <random>

const static int rowCount = 8;
const static int rowLength = rowCount + 1;
const static int maxNumbers = 100;
const static int minNumbers = -50;
static int activeProcs = 0;


using namespace std;

/*
	2x - 2y +  z = -3
	 x + 3y - 2z =  1
	3x -  y -  z =  2
*/

static double** makeSLAU()
{
	double**m = new double* [rowCount];
	for (int i = 0; i < rowCount; i++)
	{
		m[i] = new double[rowLength];
	}
	for (int i = 0; i < rowCount; i++)
	{
		for (int j = 0; j < rowLength - 1; j++) m[i][j] = rand() % maxNumbers + minNumbers;
	}

	double* x = new double[rowCount];
	for (int i = 0; i < rowCount; i++)
	{
		x[i] = rand() % maxNumbers + minNumbers;
	}
	for (int i = 0; i < rowCount; i++)
	{
		std::cout << "PRAVILNIY X" << i + 1 << " = " << x[i] << "\n";
	}

	for (int i = 0; i < rowCount; i++)
	{
		m[i][rowLength - 1] = 0;
	}

	for (int i = 0; i < rowCount; i++)
	{
		for (int j = 0; j < rowLength - 1; j++) m[i][rowLength - 1] += m[i][j] * x[j];
	}
	cout << "STARTOVAYA MATRICA\n";
	for (int i = 0; i < rowCount; i++)
	{
		for (int j = 0; j < rowLength; j++) cout << m[i][j] << " ";
		cout << "\n";
	}

	return m;
}

int main()
{
	MPI_Init(NULL, NULL);

	int processID;
	MPI_Comm_rank(MPI_COMM_WORLD, &processID);
	int numProc;
	MPI_Comm_size(MPI_COMM_WORLD, &numProc);

	activeProcs = numProc;

	if (numProc > 1)
	{
		if (processID == 0)
		{
			//double m[3][4] = { {2, -2, 1, -3}, {1, 3, -2, 1}, {3, -1, -1, 2} };
			double** m = makeSLAU();

			double sum = 0;
			double* mList = new double[rowCount * rowLength],
				* x = new double[rowCount + 1];
			for (int i = 0; i < rowCount; i++) x[i] = 0;
			x[rowCount] = -1;

			for (int i = 0; i < rowCount; i++)
			{
				for (int j = 0; j < rowLength; j++)
				{
					mList[i * rowLength + j] = m[i][j];
				}
			}

			for (int i = 0; i < rowCount - 1; i++)
			{
				int curRowCount = rowCount - i - 1;
				int* pcurRowCount = &curRowCount;
				int curRowLength = rowLength - i;
				int* pcurRowLength = &curRowLength;

				int bSize = curRowCount / (numProc - 1);
				if (bSize == 0) bSize = 1;
				int* pbSize = &bSize;

				int curListShift = bSize;

				double* firstRow = mList + (rowLength * i) + i;

				int needed = 228;
				int* pneeded = &needed;

				for (int k = 1; k < numProc && k <= curRowCount; k++)
				{
					MPI_Send(pneeded,
						1,
						MPI_INT,
						k,
						0,
						MPI_COMM_WORLD);
					//cout << "***k = " << k << "***\n";
					MPI_Send(pcurRowLength,
						1,
						MPI_INT,
						k,
						0,
						MPI_COMM_WORLD);
					//cout << "curRowLength = " << *pcurRowLength << '\n';

					if (k == numProc - 1)
					{
						bSize += (curRowCount) % (numProc - 1);
						int* pbSize = &bSize;
					}
					MPI_Send(pbSize,
						1,
						MPI_INT,
						k,
						0,
						MPI_COMM_WORLD);
					//cout << "bSize = " << *pbSize << '\n';

					MPI_Send(firstRow,
						curRowLength,
						MPI_DOUBLE,
						k,
						0,
						MPI_COMM_WORLD);
					//for (int a = 0; a < curRowLength; a++) cout << "firstRow[" << a << "] = " << firstRow[a] << '\n';

					double* curList = new double[curRowLength * curRowCount];
					for (int z = 0; z < curRowCount; z++)
					{
						for (int v = 0; v < curRowLength; v++)
						{
							curList[z * curRowLength + v] = mList[(z + i + k * curListShift) * rowLength + (v + i)];
						}
					}
					double* pcurList = curList;
					MPI_Send(pcurList,
						curRowLength * bSize,
						MPI_DOUBLE,
						k,
						0,
						MPI_COMM_WORLD);
					//for (int a = 0; a < curRowLength * bSize; a++) cout << "curList[" << a << "] = " << curList[a] << '\n';

					if (k == curRowCount && k < numProc - 1)
					{
						needed = 0;
						pneeded = &needed;
						for (int o = k + 1; o < numProc; o++)
						{
							MPI_Send(pneeded,
								1,
								MPI_INT,
								o,
								0,
								MPI_COMM_WORLD);
						}
						activeProcs = k + 1;
					}

				}
				for (int k = 1; k < activeProcs; k++)
				{
					if (k == activeProcs - 1)
					{
						double* curResList = new double[curRowLength * bSize];
						MPI_Recv(curResList,
							curRowLength * bSize,
							MPI_DOUBLE,
							k,
							0,
							MPI_COMM_WORLD,
							MPI_STATUS_IGNORE);

							for (int z = 0; z < bSize; z++)
							{
								for (int v = i; v < rowLength; v++)
								{
									mList[(z + i + k * curListShift) * rowLength + v] = curResList[z * curRowLength + v - i];
								}
							}
					}
					else
					{
						double* curResList = new double[curRowLength * curListShift];
						MPI_Recv(curResList,
							curRowLength * curListShift,
							MPI_DOUBLE,
							k,
							0,
							MPI_COMM_WORLD,
							MPI_STATUS_IGNORE);

							for (int z = 0; z < curListShift; z++)
							{
								for (int v = i; v < rowLength; v++)
								{
									mList[(z + i + k * curListShift) * rowLength + v] = curResList[z * curRowLength + v - i];
								}
							}
					}
					//for (int a = 0; a < rowCount * rowLength; a++) cout << "mList[" << a << "] = " << mList[a] << '\n';
					//cout << "curRowLength = " << curRowLength << '\n';
				}
				//cout << "\n\n";
			}
			cout << "--------------------------\n\n\n\n\n\n";
				for (int i = 0; i < rowCount; i++)
				{
					for (int j = 0; j < rowLength; j++)
					{
						m[i][j] = mList[i * rowLength + j];
					}
				}
				for (int i = 0; i < rowCount; i++)
				{
					for (int j = 0; j < rowLength; j++) cout << m[i][j] << " ";
					cout << "\n";
				}

				for (int i = rowCount - 1; i >= 0; i--)
				{
					sum = 0;
					for (int j = 0; j < rowLength; j++)
					{
						if (j != i)  sum += m[i][j] * x[j];
					}

					x[i] = -(sum / m[i][i]);
				}
				for (int i = 0; i < rowCount; i++)
				{
					std::cout << "X" << i + 1 << " = " << x[i] << "\n";
				}

			int needed = 0;
			int* pneeded = &needed;
			for (int o = 1; o < numProc; o++)
			{
				MPI_Send(pneeded,
					1,
					MPI_INT,
					o,
					0,
					MPI_COMM_WORLD);
			}
		}
		else
		{
			int ifiller1 = 0;
			int ifiller2 = 0;
			int ifiller3 = 0;
			double dfiller = 0;

			int* needed = &ifiller1;
			MPI_Recv(needed,
				1,
				MPI_INT,
				0,
				0,
				MPI_COMM_WORLD,
				MPI_STATUS_IGNORE);

			while (*needed == 228)
			{
				int* curRowLength = &ifiller2;
				int* bSize = &ifiller3;
				

				//cout << "****************************************my id = " << processID << "\n";
				MPI_Recv(curRowLength,
					1,
					MPI_INT,
					0,
					0,
					MPI_COMM_WORLD,
					MPI_STATUS_IGNORE);
				MPI_Recv(bSize,
					1,
					MPI_INT,
					0,
					0,
					MPI_COMM_WORLD,
					MPI_STATUS_IGNORE);

				double* firstRow = new double[*curRowLength];
				MPI_Recv(firstRow,
					*curRowLength,
					MPI_DOUBLE,
					0,
					0,
					MPI_COMM_WORLD,
					MPI_STATUS_IGNORE);

				double* curList = new double[*curRowLength * *bSize];
				MPI_Recv(curList,
					*curRowLength * *bSize,
					MPI_DOUBLE,
					0,
					0,
					MPI_COMM_WORLD,
					MPI_STATUS_IGNORE);


				//cout << "***PID = " << processID << "***\n";
				//cout << "curRowLength = " << *curRowLength << '\n';
				//cout << "bSize = " << *bSize << '\n';
				//for (int a = 0; a < *curRowLength; a++) cout << "firstRow[" << a << "] = " << firstRow[a] << '\n';
				//for (int a = 0; a < *curRowLength * *bSize; a++) cout << "curList[" << a << "] = " << curList[a] << '\n';

				double ** curM = new double* [*bSize];
				for (int i = 0; i < *bSize; i++)
				{
					curM[i] = new double[*curRowLength];
				}
				for (int i = 0; i < *bSize; i++)
				{
					for (int j = 0; j < *curRowLength; j++)
					{
						curM[i][j] = curList[i * *curRowLength + j];
					}
				}

				//for (int i = 0; i < *bSize; i++)
				//{
				//	for (int j = 0; j < *curRowLength; j++)
				//	{
				//		cout << curM[i][j] << " ";
				//	}
				//	cout << "\n";
				//}

				for (int i = 0; i < *bSize; i++)
				{
					double k = curM[0][i] / firstRow[0];
					for (int j = i; j < *curRowLength; j++)
					{
						curM[i][j] -= firstRow[j] * k;
					}
				}
				//for (int i = 0; i < *bSize; i++)
				//{
				//	for (int j = 0; j < *curRowLength; j++)
				//	{
				//		cout << curM[i][j] << " ";
				//	}
				//	cout << "\n";
				//}

				for (int i = 0; i < *bSize; i++)
				{
					for (int j = 0; j < *curRowLength; j++)
					{
						curList[i * rowLength + j] = curM[i][j];
					}
				}
				double* pcurList = curList;
				MPI_Send(pcurList,
					*curRowLength * *bSize,
					MPI_DOUBLE,
					0,
					0,
					MPI_COMM_WORLD);

				MPI_Recv(needed,
					1,
					MPI_INT,
					0,
					0,
					MPI_COMM_WORLD,
					MPI_STATUS_IGNORE);
			}
		}
	}
	else
	{
		cout << "1 proc only";
	}
	MPI_Finalize();
}
