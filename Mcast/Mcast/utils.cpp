#include "stdafx.h"
#include "utils.h"

void utils_TChar2Char(TCHAR *pIn, char *pOut, int maxlen)
{
	int len = WideCharToMultiByte(CP_ACP, 0, pIn, -1, NULL, 0, NULL, NULL);  
	if (len >= maxlen)
	{
		return;
	}
	
	WideCharToMultiByte(CP_ACP, 0, pIn, -1, pOut, len, NULL, NULL);
}

void utils_Char2Tchar(char *pIn, TCHAR *pOut, int maxlen)
{
	int len = MultiByteToWideChar(CP_ACP, 0, pIn, strlen(pIn)+1, NULL, 0);
	if (len >= maxlen)
	{
		return;
	}
	
	MultiByteToWideChar(CP_ACP, 0, pIn, strlen(pIn)+1, pOut, len);
}


int utils_LeastSquare(double szX[], double szY[], int size, double *pK, double *pB)
{
	double  sum_x = 0;
	double  sum_y = 0;
	double  sum_x2 = 0;
	double  sum_xy = 0;
	double  Denominator;
	
	for (int i = 0; i < size; i++) 
	{
		sum_x += szX[i];
		sum_y += szY[i];
		sum_x2 += szX[i] * szX[i];
		sum_xy += szX[i] * szY[i];
	}

	Denominator = size * sum_x2 - sum_x * sum_x;
	if (0 == Denominator)
	{
		return -1;
	}
	
	*pK = (size * sum_xy - sum_x * sum_y) / Denominator;
	*pB = (sum_x2 * sum_y - sum_x * sum_xy) / Denominator;

	return 0;
}

