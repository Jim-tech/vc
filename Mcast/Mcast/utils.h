#ifndef __UTILS_H__
#define __UTILS_H__

extern void utils_TChar2Char(TCHAR *pIn, char *pOut, int maxlen);
extern void utils_Char2Tchar(char *pIn, TCHAR *pOut, int maxlen);
extern int utils_LeastSquare(double szX[], double szY[], int size, double *pK, double *pB);


#endif /* __UTILS_H__ */
