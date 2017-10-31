#include "stdafx.h"
#include "stdio.h"
#include "libusb.h"
#include "libatecard.h"

#define USB_IO_SET_EP           0x5
#define USB_IO_GET_EP           0x85

static libusb_device          **g_usbdevs = NULL;
static libusb_device_handle    *g_usbdevhandle = NULL;

int atecard_driver_init()
{
	int ret = libusb_init(NULL);
	if (ret < 0)
	{
		printf("libusb_init() failed: %s\r\n", libusb_error_name(ret));
		return -1;
	}

	if (libusb_get_device_list(NULL, &g_usbdevs) < 0)
	{
		printf("libusb_get_device_list() failed: %s\r\n", libusb_error_name(ret));
		libusb_exit(NULL);
		return -1;
	}

	return 0;
}

void atecard_driver_deinit()
{
	libusb_free_device_list(g_usbdevs, 1);
	libusb_exit(NULL);
}

int atecard_getdevcount(int *pcount)
{
	int  ret = 0;
	int  count = 0;
	libusb_device *dev;
	struct libusb_device_descriptor desc;

	for (int i = 0; (dev = g_usbdevs[i]) != NULL; i++)
	{
		if (libusb_get_device_descriptor(dev, &desc) >= 0)
		{
			if (desc.idVendor == 0xdefa && desc.idProduct == 0x9361)
			{
				count++;
			}
		}
	}
	
	*pcount = count;
	return 0;
}

int atecard_open()
{
	int  ret = 0;
	int  count = 0;
	libusb_device *dev;
	struct libusb_device_descriptor desc;

	for (int i = 0; (dev = g_usbdevs[i]) != NULL; i++)
	{
		if (libusb_get_device_descriptor(dev, &desc) >= 0)
		{
			if (desc.idVendor == 0xdefa && desc.idProduct == 0x9361)
			{
				count++;
				ret = libusb_open(dev, &g_usbdevhandle);
				if (ret >= 0)
				{
					libusb_claim_interface(g_usbdevhandle, 4);
					return 0;
				}
			}
		}
	}

	return -1;
}

void atecard_close()
{
	if (NULL != g_usbdevhandle)
	{
		libusb_close(g_usbdevhandle);
	}
}

int atecard_io_set(unsigned char id, unsigned level)
{
	int         ret;
	int         actlen = 0;
	GPIO_CMD_S  streq;
	GPIO_CMD_S  stresp;

	if (NULL == g_usbdevhandle)
	{
		return -1;
	}

	streq.cmd = IO_GPIO_SET;
	streq.id = id;
	streq.level = level;

	ret = libusb_bulk_transfer(g_usbdevhandle, USB_IO_SET_EP, (unsigned char *)&streq, sizeof(streq), &actlen, 2000);
	if (ret < 0)
	{
		printf("send set cmd fail ret=%d\r\n", ret);
		return -1;
	}

	ret = libusb_bulk_transfer(g_usbdevhandle, USB_IO_GET_EP, (unsigned char *)&stresp, sizeof(stresp), &actlen, 2000);
	if (ret < 0)
	{
		printf("send get cmd fail ret=%d\r\n", ret);
		return -1;
	}

	return 0;
}

int atecard_io_get(unsigned char id, unsigned *plevel)
{
	int         ret;
	int         actlen = 0;
	GPIO_CMD_S  streq;
	GPIO_CMD_S  stresp;

	if (NULL == g_usbdevhandle)
	{
		return -1;
	}

	streq.cmd = IO_GPIO_GET;
	streq.id = id;

	ret = libusb_bulk_transfer(g_usbdevhandle, USB_IO_SET_EP, (unsigned char *)&streq, sizeof(streq), &actlen, 2000);
	if (ret < 0)
	{
		printf("send set cmd fail ret=%d\r\n", ret);
		return -1;
	}

	ret = libusb_bulk_transfer(g_usbdevhandle, USB_IO_GET_EP, (unsigned char *)&stresp, sizeof(stresp), &actlen, 2000);
	if (ret < 0)
	{
		printf("send get cmd fail ret=%d\r\n", ret);
		return -1;
	}

	*plevel = stresp.level;
	return 0;
}

int atecard_adc_get(unsigned short *pval)
{
	int         ret;
	int         actlen = 0;
	ADC_CMD_S   streq;
	ADC_CMD_S   stresp;

	if (NULL == g_usbdevhandle)
	{
		return -1;
	}

	streq.cmd = IO_ADC_CMD;

	ret = libusb_bulk_transfer(g_usbdevhandle, USB_IO_SET_EP, (unsigned char *)&streq, sizeof(streq), &actlen, 2000);
	if (ret < 0)
	{
		printf("send set cmd fail ret=%d\r\n", ret);
		return -1;
	}

	ret = libusb_bulk_transfer(g_usbdevhandle, USB_IO_GET_EP, (unsigned char *)&stresp, sizeof(stresp), &actlen, 2000);
	if (ret < 0)
	{
		printf("send get cmd fail ret=%d\r\n", ret);
		return -1;
	}

	*pval = stresp.val;
	return 0;
}

