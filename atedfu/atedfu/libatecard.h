#pragma once

#pragma pack(1)
typedef enum
{
	IO_RESET_CMD = 0x01,
	IO_GPIO_GET  = 0x02,
	IO_GPIO_SET  = 0x03,
	IO_GPIO_INIT = 0x04,
	IO_ADC_CMD   = 0x05,
    IO_DFU_CMD   = 0x06,	
	IO_VER_CMD   = 0x07,
}IO_CMD_E;

typedef struct
{
	unsigned char cmd;
	unsigned char error;
	unsigned char id;       //一共4个IO口，每个16pin，编号从0~63
	unsigned char dir;      //0--in, 1--out
	unsigned char level;
}GPIO_CMD_S;

typedef struct
{
	unsigned char  cmd;
	unsigned char  error;
	unsigned short val;
}ADC_CMD_S;

typedef struct
{
	unsigned char  cmd;
    unsigned char  error;    
	unsigned short val;
}DFU_CMD_S;

typedef struct
{
	unsigned char  cmd;
	unsigned char  error;
	unsigned short val;
}VER_CMD_S;

#pragma pack()


extern int atecard_driver_init();
extern void atecard_driver_deinit();
extern int atecard_open();
extern void atecard_close();
extern int atecard_getdevcount(int *pcount);
extern int atecard_io_get(unsigned char id, unsigned char *plevel);
extern int atecard_io_set(unsigned char id, unsigned char level);
extern int atecard_adc_get(unsigned short *pval);
extern int atecard_io_init(unsigned char id, unsigned dir, unsigned char level);
extern int atecard_ver_get(unsigned short *pval);
extern int atecard_dfu_enable();
