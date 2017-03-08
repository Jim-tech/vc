#ifndef __MEM_DB_H__
#define __MEM_DB_H__

typedef signed char       s8;
typedef signed short      s16;
typedef signed int        s32;
typedef unsigned char     u8;
typedef unsigned short    u16;
typedef unsigned int      u32;


#define dbgprint(...) \
    do\
    {\
        printf("[%s][%d]", __FUNCTION__, __LINE__);\
        printf(__VA_ARGS__);\
        printf("\n");\
     }while(0)

/****************************************************************************************************************************************/



#endif
