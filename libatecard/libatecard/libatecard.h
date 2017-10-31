#pragma once

#pragma pack(1)
typedef enum
{
	IO_RESET_CMD = 0x01,
	IO_GPIO_GET  = 0x02,
	IO_GPIO_SET  = 0x03,
	IO_ADC_CMD   = 0x04,
}IO_CMD_E;

typedef struct
{
	unsigned char cmd;
	unsigned char id;
	unsigned char level;
}GPIO_CMD_S;

typedef struct
{
	unsigned char  cmd;
	unsigned short val;
}ADC_CMD_S;

#pragma pack()


extern int atecard_driver_init();
extern void atecard_driver_deinit();
extern int atecard_open();
extern void atecard_close();
extern int atecard_getdevcount(int *pcount);
extern int atecard_io_get(unsigned char id, unsigned *plevel);
extern int atecard_io_set(unsigned char id, unsigned level);
extern int atecard_adc_get(unsigned short *pval);

