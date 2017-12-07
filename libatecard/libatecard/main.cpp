#include "stdafx.h"
#include "stdio.h"
#include "libusb.h"
#include "inner.h"
#include "libatecard.h"

int atecard_driver_init()
{
	return atecard_driver_init_inner();
}

void atecard_driver_deinit()
{
	atecard_driver_deinit_inner();
	return;
}

int atecard_getdevcount(int *pcount)
{
	return atecard_getdevcount_inner(pcount);
}

int atecard_open()
{
	return atecard_open_inner();
}

void atecard_close()
{
	atecard_close_inner();
	return;
}

int atecard_io_init(unsigned char id, unsigned dir, unsigned char level)
{
	return atecard_io_init_inner(id, dir, level);
}

int atecard_io_set(unsigned char id, unsigned char level)
{
	return atecard_io_set_inner(id, level);
}

int atecard_io_get(unsigned char id, unsigned char *plevel)
{
	return atecard_io_get_inner(id, plevel);
}

int atecard_adc_get(unsigned short *pval)
{
	return atecard_adc_get_inner(pval);
}

int atecard_dfu_enable()
{
	return atecard_dfu_enable_inner();
}

int atecard_ver_get(unsigned short *pval)
{
	return atecard_ver_get_inner(pval);
}