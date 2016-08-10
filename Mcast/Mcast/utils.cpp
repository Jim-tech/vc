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

static unsigned int g_utils_crc_table[256];  

/*  
 *初始化crc表,生成32位大小的crc表  
 *也可以直接定义出crc表,直接查表,  
 *但总共有256个,看着眼花,用生成的比较方便.  
 */  
static void utils_init_crc_table(void)  
{  
    unsigned int c;  
    unsigned int i, j;  
      
    for (i = 0; i < 256; i++) {  
        c = (unsigned int)i;  
        for (j = 0; j < 8; j++) {  
            if (c & 1)  
                c = 0xedb88320L ^ (c >> 1);  
            else  
                c = c >> 1;  
        }  
        g_utils_crc_table[i] = c;  
    }  
}

/*计算buffer的crc校验码*/  
unsigned int utils_calc_crc32(unsigned int crc,unsigned char *buffer, unsigned int size)  
{  
    unsigned int  i;
    unsigned char init = 0;

    if (0 == init)
    {
        utils_init_crc_table();
        init = 1;
    }
    
    for (i = 0; i < size; i++) {  
        crc = g_utils_crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);  
    }  
    return crc ;  
}