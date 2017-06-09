/*********************************************************************
*
* File Name :  eeprom-common.h
* Work Group   :   Common Platform GROUP
* Creator      :   chen.pengfei
* Create Date  :   2015/07/13
* Description  :   Common Library
* Version      :   1.0
*
*-----------------------------------------------------------------------------
* $Log: eeprom-common.h,v $
* Revision 1.0  2015/07/13  chen.pengfei
* Revision 1.1  2015/8/17   css
* 1. Create the file.
* 2. Modify by css 
*********************************************************************/
#ifndef _EEPROM_COMMON_H_
#define _EEPROM_COMMON_H_

#define WRITE           0x0
#define READ            0x1

#define MAX_BUFFER_LEN  64
#define INT_LEN         4

#define MAC_DATA_LEN                                (6)       //length of mac address in EEPROM
#define SN_DATA_LEN                                 (20)      //length of SN in EEPROM
#define POWER1_DATA_LEN                             (INT_LEN) //length of Power1 in EEPROM
#define POWER2_DATA_LEN                             (INT_LEN) //length of Power2 in EEPROM
#define FREQ_OFF_DATA_LEN                           (INT_LEN) //length of Frequency Offset in EEPROM
#define AGING_FLAG_TOTAL_DATA_LEN                   (INT_LEN) //length of Flag in EEPROM
#define AGING_FLAG_SUB_DATA_LEN                     (INT_LEN) //length of Sub Flag in EEPROM
#define AGING_FLAG_STAGE_DATA_LEN                   (INT_LEN) //length of Stage Flag in EEPROM
#define AGING_FLAG_ITEMMASK_DATA_LEN                (INT_LEN) //length of the mask for aging item
#define FLAG_PRODUCTION_VERSION_LEN                 (INT_LEN) //length of production version Flag in EEPROM
#define HARDWARE_VERSION_DATA_LEN                   (20)      //length of Hardware Version in EEPROM
#define FLAG_GPS_INFO_LEN                           (INT_LEN) //length of GPS info in EEPROM
#define FLAG_BOOM_LEN                               (INT_LEN) //length of boot mode in EEPROM 
#define FLAG_UPGRADE_INFO_LEN                       (INT_LEN) //length of upgrade info in EEPROM 
#define FLAG_RX1_LEN                                (INT_LEN) //length of upgrade info in EEPROM 
#define FLAG_RX2_LEN                                (INT_LEN) //length of upgrade info in EEPROM 
#define BOARD_TYPE_LEN                              (8) //length of upgrade info in EEPROM 
#define BARCODE_LEN                                 (19) //length of upgrade info in EEPROM 
#define ITEM_BOM_LEN                                (10) //length of upgrade info in EEPROM 
#define DESCRIPTION_LEN                             (200) //length of upgrade info in EEPROM 
#define MANUFACTURED_LEN                            (10) //length of upgrade info in EEPROM 
#define VENDOR_NAME_LEN                             (10) //length of upgrade info in EEPROM 
#define ISSUE_NUM_LEN                               (2) //length of upgrade info in EEPROM 
#define CLEI_CODE_LEN                               (10) //length of upgrade info in EEPROM 
#define BOM_LEN                                     (10) //length of upgrade info in EEPROM 

#define CONFIG_AGING_FLAG                           (0x600387)
#define CONFIG_AGING_FLAG_OK		       		    (CONFIG_AGING_FLAG << 8)
#define CONFIG_AGING_FLAG_TOTAL_CNT_MAX             (100)
#define CONFIG_AGING_FLAG_TOTAL_CNT_MASK            (0xFF)
#define CONFIG_AGING_FLAG_SUB_DDR_MASK              (0x1)
#define CONFIG_AGING_FLAG_SUB_NAND_MASK             (0x2)
#define CONFIG_AGING_FLAG_SUB_ETH_MASK              (0x4)
#define CONFIG_AGING_FLAG_SUB_GPS_MASK              (0x8)
#define CONFIG_AGING_FLAG_SUB_USIM_MASK             (0x10)
#define CONFIG_AGING_FLAG_SUB_WATCHDOG_MASK         (0x20)

//弘浩明传
#define HW_VERSION_HHMC             "HM-PB-T30_V3.1"
//永鼎
#define HW_VERSION_YD               "ECPBTS801_V2.0"
//国人
#define HW_VERSION_GR               "GRBS20_V2.0"
//特发
#define HW_VERSION_TF               "IAC4500_V2.0"

typedef enum
{
	E_TYPE_UNKOWN               =-1,	//unkown operation type
	E_TYPE_MAC                  = 0,	//read/write mac from eeprom
 	E_TYPE_SN                   = 1,	//read/write SN from eeprom
 	E_TYPE_POWER1               = 2,	//read/write POWER1 from eeprom
 	E_TYPE_POWER2               = 3,	//read/write POWER2 from eeprom
 	E_TYPE_FREQOFFSET           = 4,	//read/write Frequency Offset from eeprom
 	E_TYPE_FLAG                 = 5,	//read/write Flag from eeprom
 	E_TYPE_FLAGSUB              = 6,	//read/write Sub Flag from eeprom
 	E_TYPE_FLAGSTAGE            = 7,	//read/write ItemMask Flag from eeprom
	E_TYPE_FLAG_ITEMMASK        = 8,	//read/write Stage Flag from eeprom
	E_TYPE_FLAG_PROCUCTION_VER  = 9, 	//read/write production version Flag from eeprom
	E_TYPE_HARDWARE_VER         = 10,	//read/write Hardware Version from eeprom
	E_TYPE_GPS_INFO             = 11,   //read/write GPS info from eeprom
	E_TYPE_BOOTM                = 12,   //read/write boot mode from eeprom
	E_TYPE_UPGRADE_INFO         = 13,   //read/write upgrade info from eeprom
	E_TYPE_RX1                  = 14,   //read/write RX1 from eeprom
	E_TYPE_RX2                  = 15,   //read/write RX2 from eeprom
	E_TYPE_BTYPE                = 16,   //read/write board type from eeprom
	E_TYPE_ELABEL               = 17,   //read/write elabel from eeprom
}E_RW_OP;

typedef enum
{
	E_OK                        = 0,    //API interface call OK
 	E_ERROR                     = -1,   //API interface call EROOR
 	E_INVALID_ARGS              = -2,   //API interface call return invalid argument
 	E_INVALID_OPTYPE            = -3,   //API interface call return invalid operation type
 	E_DEVIOCTL_ERROR            = -4,   //API Intel ioctl error
}E_RET;

typedef enum
{
	GPS_NOEXIST   = 0x0,//GPS Device not Exist
	GPS_PARNONE   = 0x1,//GPS Device None Parity Check
	GPS_PARENABLE = 0x2,//GPS Device Using Parity Check
	GPS_UNKOWN	  = 0x100,//GPS Device Unkown
}GPSPRECONF;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_eeprom
{
	E_RW_OP rw_op_type;                 //read/write operation type
	int rwflags;                        //read/write flags
	unsigned char buf[MAX_BUFFER_LEN];  //read/write buffer
	int len;                            //read/write buffer length
}T_EEPROM;

/*************************************************************
 * Routine:  eeprom_helper
 * Function:  eeprom read/write helper function
 * Input  : peeprom{T_EEPROM *}:application struct T_EEPROM
 * Output : >=0:Successlly;Other:Failed
 * Author : chenpf
 * Date   : 2015-07-24
 *************************************************************/
 int eeprom_helper(T_EEPROM *peeprom);

int eeprom_read(int addr, int length, unsigned char *pdata);
int eeprom_write(int addr, int length, unsigned char *pdata);

#define MSG_NUM			0x2

#if defined(T2200) || defined(T2K)
#define EEPROM_ADDR		0x50
#else
#define EEPROM_ADDR		0x50
#endif//#if defined(T2200) || defined(T2K)

#define EEPROM_ADDR_LEN	0x2

/* 8207 RF */
#define RF_DATA_LEN		3
#define RF_ARRAY_LEN	RF_DATA_LEN+EEPROM_ADDR_LEN

/* MAC Addr */
#define MAC_ARRAY_LEN	MAC_DATA_LEN+EEPROM_ADDR_LEN
//mac base address 
#define MAC_BASE_ADDR	0x6800

/* SN */
#define SN_ARRAY_LEN	                    (SN_DATA_LEN+EEPROM_ADDR_LEN)
#define SN_BASE_ADDR	                    (MAC_BASE_ADDR + 16)

//Power1
#define POWER1_BASE_ADDR                    ((SN_BASE_ADDR) + SN_DATA_LEN + 12)

//Power2
#define POWER2_BASE_ADDR                    ((POWER1_BASE_ADDR) + POWER1_DATA_LEN)

//频偏
#define FREQ_OFF_BASE_ADDR                  ((POWER2_BASE_ADDR) + POWER2_DATA_LEN)

//flag
#define AGING_FLAG_TOTAL_BASE_ADDR          ((FREQ_OFF_BASE_ADDR) + 8)

//flag_sub
#define AGING_FLAG_SUB_BASE_ADDR            ((AGING_FLAG_TOTAL_BASE_ADDR) + AGING_FLAG_TOTAL_DATA_LEN)

//flag_stage
#define AGING_FLAG_STAGE_BASE_ADDR          ((AGING_FLAG_SUB_BASE_ADDR) + AGING_FLAG_SUB_DATA_LEN)

//flag_item_mask
#define AGING_FLAG_ITEMMASK_BASE_ADDR       ((AGING_FLAG_STAGE_BASE_ADDR) + AGING_FLAG_STAGE_DATA_LEN)

//flag_production_version
#define FLAG_PRODUCTION_VERSION_BASE_ADDR   ((AGING_FLAG_ITEMMASK_BASE_ADDR) + AGING_FLAG_ITEMMASK_DATA_LEN)

//Hardware Version
#define HARDWARE_VERSION_BASE_ADDR          ((FLAG_PRODUCTION_VERSION_BASE_ADDR) + FLAG_PRODUCTION_VERSION_LEN)
#define HARDWARE_VERSION_ARRAY_LEN          (HARDWARE_VERSION_DATA_LEN + EEPROM_ADDR_LEN)

//GPS info
#define FLAG_GPS_INFO_BASE_ADDR             ((HARDWARE_VERSION_BASE_ADDR) + HARDWARE_VERSION_DATA_LEN)

//BOOTM
#define FLAG_BOOTM_BASE_ADDR                ((FLAG_GPS_INFO_BASE_ADDR) + FLAG_GPS_INFO_LEN)

//Upgrade info
#define FLAG_UPGRADE_INFO_BASE_ADDR         ((FLAG_BOOTM_BASE_ADDR) + FLAG_BOOM_LEN)

//RX1
#define FLAG_RX1_BASE_ADDR                  ((FLAG_UPGRADE_INFO_BASE_ADDR) + FLAG_UPGRADE_INFO_LEN)
//RX2
#define FLAG_RX2_BASE_ADDR                  ((FLAG_RX1_BASE_ADDR) + FLAG_RX1_LEN)
//board type
#define BOARD_TYPE_BASE_ADDR                ((FLAG_RX2_BASE_ADDR) + FLAG_RX2_LEN)
//bar code
#define BARCODE_BASE_ADDR                   ((BOARD_TYPE_BASE_ADDR) + BOARD_TYPE_LEN)
#define ITEM_BOM_BASE_ADDR                  ((BARCODE_BASE_ADDR) + BARCODE_LEN)
#define DESCRIPTION_BASE_ADDR               ((ITEM_BOM_BASE_ADDR) + ITEM_BOM_LEN)
#define MANUFACTURED_BASE_ADDR              ((DESCRIPTION_BASE_ADDR) + DESCRIPTION_LEN)
#define VENDOR_BASE_ADDR                    ((MANUFACTURED_BASE_ADDR) + MANUFACTURED_LEN)
#define ISSUE_BASE_ADDR                     ((VENDOR_BASE_ADDR) + VENDOR_NAME_LEN)
#define CLEI_BASE_ADDR                      ((ISSUE_BASE_ADDR) + ISSUE_NUM_LEN)
#define BOM_BASE_ADDR                       ((CLEI_BASE_ADDR) + CLEI_CODE_LEN)

#ifdef __cplusplus
}
#endif

#endif//#ifndef _EEPROM_COMMON_H_

