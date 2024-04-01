#include <iostream>
#include "mpi.h"

const static int mSize = 3;

using namespace std;

/*
	2x - 2y +  z = -3
	 x + 3y - 2z =  1
	3x -  y -  z =  2
*/

int main()
{
	MPI_Init(NULL, NULL);

	int processID;
	MPI_Comm_rank(MPI_COMM_WORLD, &processID);
	int numProc;
	MPI_Comm_size(MPI_COMM_WORLD, &numProc);

	if (numProc > 1)
	{
		int blockSize = mSize / (numProc - 1);

		if (processID == 0)
		{
			double m[3][4] = { {2, -2, 1, -3}, {1, 3, -2, 1}, {3, -1, -1, 2} };
			double sum = 0;
			double* mList = new double[mSize * (mSize + 1)],
				* x = new double[mSize + 1],
				* filler = new double[mSize * blockSize];
			for (int i = 0; i < mSize * blockSize; i++) filler[i] = 0;
			for (int i = 0; i < mSize; i++) x[i] = 0;
			x[mSize] = -1;

			for (int i = 0; i < mSize; i++)
			{
				for (int j = 0; j < mSize; j++)
				{
					mList[(blockSize) * (mSize + 1) + i * mSize + j] = m[i][j];
				}
			}

			for (int i = 0; i < mSize - 1; i++)
			{
				for (int j = i + 1; j < mSize; j++)
				{				
					for (int k = 1; k < numProc && k < mSize - i + 1; k++)
					{
						int* bfStart = &i;
						MPI_Send(bfStart,
							1,
							MPI_INT,
							k,
							0,
							MPI_COMM_WORLD);
						bfStart = mList + (mSize + 1) * 
						MPI_Send(bfStart,
							mSize - i,
							MPI_DOUBLE,
							k,
							0,
							MPI_COMM_WORLD);
					}

					//MPI_Scatter(mList,
					//	mSize * blockSize,
					//	MPI_DOUBLE,
					//	filler,
					//	mSize * blockSize,
					//	MPI_DOUBLE,
					//	0,
					//	MPI_COMM_WORLD);

					MPI_Gather(filler,
						mSize * blockSize,
						MPI_DOUBLE,
						mList,
						mSize * blockSize,
						MPI_DOUBLE,
						0,
						MPI_COMM_WORLD);

					double k = mList[(blockSize + j) * (mSize + 1) + i] / mList[(blockSize + i) * (mSize + 1) + i];
					for (int h = i; h < mSize + 1; h++)
					{
						m[j][h] -= m[i][h] * k;
					}
				}
			}

			for (int i = 0; i < mSize; i++)
			{
				for (int j = 0; j < mSize + 1; j++)
				{
					m[i][j] = mList[(blockSize + i) * mSize + j];
				}
			}
			for (int i = 0; i < mSize; i++)
			{
				for (int j = 0; j < mSize + 1; j++) cout << m[i][j] << " ";
				cout << "\n";
			}

			for (int i = mSize - 1; i >= 0; i--)
			{
				sum = 0;
				for (int j = 0; j < mSize + 1; j++)
				{
					if (j != i)  sum += m[i][j] * x[j];
				}

				x[i] = -(sum / m[i][i]);
			}
			for (int i = 0; i < mSize; i++)
			{
				std::cout << "X" << i + 1 << " = " << x[i] << "\n";
			}
		}
		else
		{
			double* mList = new double[(mSize + blockSize) * (mSize + 1)],
				** m = new double* [blockSize],
				* filler = new double[mSize * blockSize];

			for (int i = 0; i < mSize * blockSize; i++) filler[i] = 0;
			for (int i = 0; i < blockSize; i++)
			{
				m[i] = new double[mSize + 1];
			}

			MPI_Scatter(filler,
				mSize * blockSize,
				MPI_DOUBLE,
				mList,
				mSize * blockSize,
				MPI_DOUBLE,
				0,
				MPI_COMM_WORLD);

			for (int i = 0; i < blockSize; i++)
			{
				for (int j = 0; j < mSize; j++)
				{
					m[i][j] = mList[i * mSize + j];
				}
			}

			MPI_Gather(resultList,
				mSize * blockSize,
				MPI_DOUBLE,
				filler,
				mSize * blockSize,
				MPI_DOUBLE,
				0,
				MPI_COMM_WORLD);
		}
	}
	MPI_Finalize();
}
