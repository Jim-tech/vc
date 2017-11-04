#ifndef __CALIBRATIONTABLE_H__
#define __CALIBRATIONTABLE_H__

#define VIRTUAL_EEPROM               "__virtual_EEPROM__"
#define EEPROM_SIZE                  0x10000

#define EEPROM_CAL_TABLE_BASE_ADDR    0x7800
#define CAL_TABLE_DATA_MAX_LEN       (8*1024)


typedef enum
{
    CAL_DESC_TBL            = 0,        /*描述表  CalDescTbl_S */
    CAL_GLOBAL_PARA_TBL     = 1,        /*全局参数表(存储一些全局参数)，cal_global_para_s  */
    CAL_FREQ_TX1_TBL        = 2,        /*下行通道1频补  cal_delta_s */
    CAL_FREQ_TX2_TBL        = 3,        /*下行通道2频补  cal_delta_s */
    CAL_FREQ_RX1_TBL        = 4,        /*上行通道1频补  cal_delta_s */
    CAL_FREQ_RX2_TBL        = 5,        /*上行通道2频补  cal_delta_s */
    CAL_FREQ_FB1_TBL        = 6,        /*反馈通道1频补  cal_delta_s */
    CAL_FREQ_FB2_TBL        = 7,        /*反馈通道2频补  cal_delta_s */
    CAL_TEMP_TX1_TBL        = 8,        /*下行通道1温补  cal_delta_s */
    CAL_TEMP_TX2_TBL        = 9,        /*下行通道2温补  cal_delta_s */
    CAL_TEMP_RX1_TBL        = 10,       /*上行通道1温补  cal_delta_s */
    CAL_TEMP_RX2_TBL        = 11,       /*上行通道2温补  cal_delta_s */
    CAL_TEMP_FB1_TBL        = 12,       /*反馈通道1温补  cal_delta_s */
    CAL_TEMP_FB2_TBL        = 13,       /*反馈通道2温补  cal_delta_s */
    CAL_VGS_C1_P1_TBL       = 14,       /*通道1，功放1温补 cal_delta_s */
    CAL_VGS_C1_P2_TBL       = 15,       /*通道1，功放2温补 cal_delta_s */
    CAL_VGS_C1_P3_TBL       = 16,       /*通道1，功放3温补 cal_delta_s */
    CAL_VGS_C2_P1_TBL       = 17,       /*通道2，功放1温补 cal_delta_s */
    CAL_VGS_C2_P2_TBL       = 18,       /*通道2，功放2温补 cal_delta_s */
    CAL_VGS_C2_P3_TBL       = 19,       /*通道2，功放3温补 cal_delta_s */
    CAL_PWR_VDS_TBL         = 20,       /*功率回退，漏极电压回退表  cal_delta_s */
    CAL_PWR_VGS_C1_P1_TBL   = 21,       /*通道1功放1栅压回退 cal_delta_s */
    CAL_PWR_VGS_C1_P2_TBL   = 22,       /*通道1功放2栅压回退 cal_delta_s */
    CAL_PWR_VGS_C1_P3_TBL   = 23,       /*通道1功放3栅压回退 cal_delta_s */
    CAL_PWR_VGS_C2_P1_TBL   = 24,       /*通道2功放1栅压回退 cal_delta_s */
    CAL_PWR_VGS_C2_P2_TBL   = 25,       /*通道2功放2栅压回退 cal_delta_s */
    CAL_PWR_VGS_C2_P3_TBL   = 26,       /*通道2功放3栅压回退 cal_delta_s */
    CAL_FLTR_PARAM_C1_TBL   = 27,       /*通道1滤波器参数 filter_param_s */
    CAL_FLTR_PARAM_C2_TBL   = 28,       /*通道2滤波器参数 filter_param_s */
	CAL_FREQ_FB3_TBL = 29,
	CAL_FREQ_FB4_TBL = 30,
	CAL_TEMP_TX3_TBL = 31,
	CAL_TEMP_TX4_TBL = 32,
	CAL_TEMP_RX3_TBL = 33,
	CAL_TEMP_RX4_TBL = 34,
	CAL_TEMP_FB3_TBL = 35,
	CAL_TEMP_FB4_TBL = 36,
	CAL_VGS_C3_P1_TBL = 37,
	CAL_VGS_C3_P2_TBL = 38,
	CAL_VGS_C3_P3_TBL = 39,
	CAL_VGS_C4_P1_TBL = 40,
	CAL_VGS_C4_P2_TBL = 41,
	CAL_VGS_C4_P3_TBL = 42,

	CAL_TBL_LAST = CAL_VGS_C4_P3_TBL,
    CAL_TBL_MAX             = 128        /*最多128张表*/
}CAL_TBLID_E;


#define CAL_TABLE_NAME \
{ \
    "0_CAL_DESC_TBL", \
    "1_CAL_GLOBAL_PARA_TBL", \
    "2_CAL_FREQ_TX1_TBL", \
    "3_CAL_FREQ_TX2_TBL", \
    "4_CAL_FREQ_RX1_TBL", \
    "5_CAL_FREQ_RX2_TBL", \
    "6_CAL_FREQ_FB1_TBL", \
    "7_CAL_FREQ_FB2_TBL", \
    "8_CAL_TEMP_TX1_TBL", \
    "9_CAL_TEMP_TX2_TBL", \
    "10_CAL_TEMP_RX1_TBL", \
    "11_CAL_TEMP_RX2_TBL", \
    "12_CAL_TEMP_FB1_TBL", \
    "13_CAL_TEMP_FB2_TBL", \
    "14_CAL_VGS_C1_P1_TBL", \
    "15_CAL_VGS_C1_P2_TBL", \
    "16_CAL_VGS_C1_P3_TBL", \
    "17_CAL_VGS_C2_P1_TBL", \
    "18_CAL_VGS_C2_P2_TBL", \
    "19_CAL_VGS_C2_P3_TBL", \
    "20_CAL_PWR_VDS_TBL", \
    "21_CAL_PWR_VGS_C1_P1_TBL", \
    "22_CAL_PWR_VGS_C1_P2_TBL", \
    "23_CAL_PWR_VGS_C1_P3_TBL", \
    "24_CAL_PWR_VGS_C2_P1_TBL", \
    "25_CAL_PWR_VGS_C2_P2_TBL", \
    "26_CAL_PWR_VGS_C2_P3_TBL", \
    "27_CAL_FLTR_PARAM_C1_TBL", \
    "28_CAL_FLTR_PARAM_C2_TBL", \
    "29_CAL_FREQ_FB3_TBL", \
    "30_CAL_FREQ_FB4_TBL", \
    "31_CAL_TEMP_TX3_TBL", \
    "32_CAL_TEMP_TX4_TBL", \
    "33_CAL_TEMP_RX3_TBL", \
    "34_CAL_TEMP_RX4_TBL", \
    "35_CAL_TEMP_FB3_TBL", \
    "36_CAL_TEMP_FB4_TBL", \
    "37_CAL_VGS_C3_P1_TBL", \
    "38_CAL_VGS_C3_P2_TBL", \
    "39_CAL_VGS_C3_P3_TBL", \
    "40_CAL_VGS_C4_P1_TBL", \
    "41_CAL_VGS_C4_P2_TBL", \
    "42_CAL_VGS_C4_P3_TBL", \
}

typedef enum
{
    CAL_K_TX1               = 0,       /*通道1下行K值  */
    CAL_B_TX1               = 1,       /*通道1下行B值  */
    CAL_K_TX2               = 2,       /*通道2下行K值  */
    CAL_B_TX2               = 3,       /*通道2下行B值  */
    CAL_K_FB1               = 4,       /*通道1反向K值  */
    CAL_B_FB1               = 5,       /*通道1反向B值  */
    CAL_K_FB2               = 6,       /*通道2反向K值  */
    CAL_B_FB2               = 7,       /*通道2反向B值  */
    CAL_LNA_GAIN            = 8,       /*LNA增益修正值, 单位0.1db 已废弃，保留兼容老版本*/
    ABS_LNA_GAIN            = 9,       /*LNA增益绝对值, 单位0.1db */
    RX1_PATH_GAIN           = 10,      /*上行通道1增益绝对值, 单位0.1db */
    RX2_PATH_GAIN           = 11,      /*上行通道2增益绝对值, 单位0.1db */

    CAL_GBL_LAST            = RX2_PATH_GAIN,
    CAL_GLB_MAX             = 128
}CAL_GLOBAL_PARA_E;

#define CAL_TBL_MAGIC        0xA5
#define CAL_TBL_VER          2
#define CAL_TBL_MAX_LEN      1024

#define CAL_GAIN_ACCURACY    10    /*增益精度 0.1db  */
#define CAL_VGS_ACCURACY     1     /*栅压精度 1mV  */
#define CAL_K_ACCURACY       100   /*k值精度  0.01  */

#pragma pack(1)
typedef struct
{
    u8          magic;
    u8          version;
    u16         offset;
    u8          recordcnt;
    u8          recordlen;
    u16         checksum;
    s32         cal_start;
    s16         cal_step;
    s16         value_base;
}caltbl_desc_s;

typedef struct 
{
    s32        para_val;
}cal_global_para_s;

typedef struct 
{
    s16        delta;
}cal_delta_s;

typedef struct 
{
    s8         delay;         /*unit:ns  */
    s8         attenu;        /*unit:0.1db  */
}filter_param_s;

typedef struct
{
    u8          version;
    u8          recordcnt;
    u8          recordlen;
    u8          resv;
    s32         cal_start;
    s16         cal_step;
    s16         value_base;
    u8          data[CAL_TBL_MAX_LEN];
}calsave_s;
#pragma pack()

extern u32 ru_alloc_caltbl(u16 len, u16 *poffset);
extern u32 ru_check_caltbl(u8 table_id);
extern u32 ru_check_caltbl_desc(void);
extern u32 ru_get_caltbl_len(u8 table_id, u16 *plen);
extern u32 ru_get_caltbl_offset(u8 table_id, u16 *poffset);
extern u32 ru_save_caldata(u8 table_id, calsave_s *ppara);
extern u32 ru_init_caltbl(void);
extern u32 ru_reset_caltbl(void);
extern u32 ru_set_caltbl_invalid(u8 table_id);
extern u32 ru_trim_caltbl(void);
extern u32 ru_update_caltbl(u8 table_id);
extern u32 ru_save_globalpara(CAL_GLOBAL_PARA_E type, s32 value);
extern u32 ru_read_caltbl(u8 table_id, calsave_s *pcaltbl);
u32 ru_read_globalpara(CAL_GLOBAL_PARA_E type, s32 *pvalue);
u32 ru_dump_caltbl(char *pfile);
bool ru_caltbl_isvalid(u8 table_id);
int ru_get_caltbl_value(u8 table_id, int key);


#endif /* __CALIBRATIONTABLE_H__ */
