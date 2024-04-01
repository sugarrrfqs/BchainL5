#include <iostream>
#include "mpi.h"

static int msize = 3;

using namespace std;

int main()
{
	/*
		2x - 2y +  z = -3
		 x + 3y - 2z =  1
		3x -  y -  z =  2
	*/
	double m[3][4] = { {2, -2, 1, -3}, {1, 3, -2, 1}, {3, -1, -1, 2} };
	double* x = new double[msize + 1];
	double sum = 0;

	for (int i = 0; i < msize; i++) x[i] = 0;
	x[msize] = -1;

	for (int i = 0; i < msize - 1; i++)
	{
#pragma omp parallel for
		for (int j = i + 1; j < msize; j++)
		{
			double k = m[j][i] / m[i][i];
			for (int h = i; h < msize + 1; h++)
			{
				m[j][h] -= m[i][h] * k;
			}
		}
	}
	for (int i = 0; i < msize; i++)
	{
		for (int j = 0; j < msize + 1; j++) cout << m[i][j] << " ";
		cout << "\n";
	}

	for (int i = msize - 1; i >= 0; i--)
	{
		sum = 0;
		for (int j = 0; j < msize + 1; j++)
		{
			if (j != i)  sum += m[i][j] * x[j];
		}

		x[i] = -(sum / m[i][i]);
	}
	for (int i = 0; i < msize; i++)
	{
		std::cout << "X" << i + 1 << " = " << x[i] << "\n";
	}
}
