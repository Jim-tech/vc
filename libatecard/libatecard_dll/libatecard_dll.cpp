// libatecard_dll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "..\libatecard\inner.h"
#include "libatecard_dll.h"


// This is an example of an exported variable
LIBATECARD_DLL_API int nlibatecard_dll=0;

// This is an example of an exported function.
LIBATECARD_DLL_API int fnlibatecard_dll(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see libatecard_dll.h for the class definition
Clibatecard_dll::Clibatecard_dll()
{
	return;
}


LIBATECARD_DLL_API int atecard_driver_init()
{
	return atecard_driver_init_inner();
}

LIBATECARD_DLL_API void atecard_driver_deinit()
{
	atecard_driver_deinit_inner();
	return;
}

LIBATECARD_DLL_API int atecard_getdevcount(int *pcount)
{
	return atecard_getdevcount_inner(pcount);
}

LIBATECARD_DLL_API int atecard_open()
{
	return atecard_open_inner();
}

LIBATECARD_DLL_API void atecard_close()
{
	atecard_close_inner();
	return;
}

LIBATECARD_DLL_API int atecard_io_init(unsigned char id, unsigned dir, unsigned char level)
{
	return atecard_io_init_inner(id, dir, level);
}

LIBATECARD_DLL_API int atecard_io_set(unsigned char id, unsigned char level)
{
	return atecard_io_set_inner(id, level);
}

LIBATECARD_DLL_API int atecard_io_get(unsigned char id, unsigned char *plevel)
{
	return atecard_io_get_inner(id, plevel);
}

LIBATECARD_DLL_API int atecard_adc_get(unsigned short *pval)
{
	return atecard_adc_get_inner(pval);
}

LIBATECARD_DLL_API int atecard_dfu_enable()
{
	return atecard_dfu_enable_inner();
}

LIBATECARD_DLL_API int atecard_ver_get(unsigned short *pval)
{
	return atecard_ver_get_inner(pval);
}