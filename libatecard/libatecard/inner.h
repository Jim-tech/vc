#ifndef __INNER_H__
#define __INNER_H__

extern int atecard_adc_get_inner(unsigned short *pval);
extern void atecard_close_inner();
extern int atecard_dfu_enable_inner();
extern void atecard_driver_deinit_inner();
extern int atecard_driver_init_inner();
extern int atecard_getdevcount_inner(int *pcount);
extern int atecard_io_get_inner(unsigned char id, unsigned char *plevel);
extern int atecard_io_init_inner(unsigned char id, unsigned dir, unsigned char level);
extern int atecard_io_set_inner(unsigned char id, unsigned char level);
extern int atecard_open_inner();
extern int atecard_ver_get_inner(unsigned short *pval);


#endif /* __INNER_H__ */

