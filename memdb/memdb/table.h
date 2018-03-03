#ifndef __CALIBRATIONTABLE_H__
#define __CALIBRATIONTABLE_H__

#define VIRTUAL_EEPROM               "__virtual_EEPROM__"
#define EEPROM_SIZE                  0x10000

#define EEPROM_CAL_TABLE_BASE_ADDR    0x7800
#define CAL_TABLE_DATA_MAX_LEN       (8*1024)


typedef enum
{
    CAL_DESC_TBL             = 0,        /*描述表  CalDescTbl_S */
    CAL_GLOBAL_PARA_TBL      = 1,        /*全局参数表(存储一些全局参数)，cal_global_para_s  */
    CAL_FREQ_TX0_TBL         = 2,        /*下行通道0频补  cal_delta_s */
    CAL_FREQ_TX1_TBL         = 3,        /*下行通道1频补  cal_delta_s */
    CAL_FREQ_RX0_TBL         = 4,        /*上行通道0频补  cal_delta_s */
    CAL_FREQ_RX1_TBL         = 5,        /*上行通道1频补  cal_delta_s */
    CAL_FREQ_REF0_TBL        = 6,        /*反射通道0频补  cal_delta_s */
    CAL_FREQ_REF1_TBL        = 7,        /*反射通道1频补  cal_delta_s */
    CAL_TEMP_TX0_TBL         = 8,        /*下行通道0温补  cal_delta_s */
    CAL_TEMP_TX1_TBL         = 9,        /*下行通道1温补  cal_delta_s */
    CAL_TEMP_RX0_TBL         = 10,       /*上行通道0温补  cal_delta_s */
    CAL_TEMP_RX1_TBL         = 11,       /*上行通道1温补  cal_delta_s */
    CAL_TEMP_REF0_TBL        = 12,       /*反射通道0温补  cal_delta_s */
    CAL_TEMP_REF1_TBL        = 13,       /*反射通道1温补  cal_delta_s */
    CAL_VGS_C0_P0_TBL        = 14,       /*通道0，功放0温补 cal_delta_s */
    CAL_VGS_C0_P1_TBL        = 15,       /*通道0，功放1温补 cal_delta_s */
    CAL_VGS_C0_P2_TBL        = 16,       /*通道0，功放2温补 cal_delta_s */
    CAL_VGS_C1_P0_TBL        = 17,       /*通道1，功放0温补 cal_delta_s */
    CAL_VGS_C1_P1_TBL        = 18,       /*通道1，功放1温补 cal_delta_s */
    CAL_VGS_C1_P2_TBL        = 19,       /*通道1，功放2温补 cal_delta_s */
    CAL_PWR_VDS_TBL          = 20,       /*功率回退，漏极电压回退表  cal_delta_s */
    CAL_PWR_VGS_C0_P0_TBL    = 21,       /*通道0功放0栅压回退 cal_delta_s */
    CAL_PWR_VGS_C0_P1_TBL    = 22,       /*通道0功放1栅压回退 cal_delta_s */
    CAL_PWR_VGS_C0_P2_TBL    = 23,       /*通道0功放2栅压回退 cal_delta_s */
    CAL_PWR_VGS_C1_P0_TBL    = 24,       /*通道1功放0栅压回退 cal_delta_s */
    CAL_PWR_VGS_C1_P1_TBL    = 25,       /*通道1功放1栅压回退 cal_delta_s */
    CAL_PWR_VGS_C1_P2_TBL    = 26,       /*通道1功放2栅压回退 cal_delta_s */
    CAL_FLTR_PARAM_TX_C0_TBL = 27,       /*通道0滤波器参数tx filter_param_s */
    CAL_FLTR_PARAM_TX_C1_TBL = 28,       /*通道1滤波器参数tx filter_param_s */
    CAL_FREQ_TX0_FB_TBL      = 29,       /*下行反馈通道0频补  cal_delta_s */
    CAL_FREQ_TX1_FB_TBL      = 30,       /*下行反馈通道1频补  cal_delta_s */
    CAL_FLTR_PARAM_RX_C0_TBL = 31,       /*通道0滤波器参数rx filter_param_s */
    CAL_FLTR_PARAM_RX_C1_TBL = 32,       /*通道1滤波器参数rx filter_param_s */
    CAL_TEMP_TX0_FB_TBL      = 33,       /*下行反馈通道0温补  cal_delta_s */
    CAL_TEMP_TX1_FB_TBL      = 34,       /*下行反馈通道1温补  cal_delta_s */
    UN_USED_TABLE_C          = 35,       /*unused  */
    UN_USED_TABLE_D          = 36,       /*unused  */
    CAL_VGS_C0_P3_TBL        = 37,       /*通道0，功放3温补 cal_delta_s */
    CAL_VGS_C1_P3_TBL        = 38,       /*通道1，功放3温补 cal_delta_s */
    CAL_PWR_VGS_C0_P3_TBL    = 39,       /*通道0功放3栅压回退 cal_delta_s */
    CAL_PWR_VGS_C1_P3_TBL    = 40,       /*通道1功放3栅压回退 cal_delta_s */

    CAL_TBL_LAST = CAL_PWR_VGS_C1_P3_TBL,
    CAL_TBL_MAX             = 128        /*最多128张表*/
}CAL_TBLID_E;

#define CAL_TABLE_NAME \
{ \
    "0_CAL_DESC_TBL", \
    "1_CAL_GLOBAL_PARA_TBL", \
    "2_CAL_FREQ_TX0_TBL", \
    "3_CAL_FREQ_TX1_TBL", \
    "4_CAL_FREQ_RX0_TBL", \
    "5_CAL_FREQ_RX1_TBL", \
    "6_CAL_FREQ_REF0_TBL", \
    "7_CAL_FREQ_REF1_TBL", \
    "8_CAL_TEMP_TX0_TBL", \
    "9_CAL_TEMP_TX1_TBL", \
    "10_CAL_TEMP_RX0_TBL", \
    "11_CAL_TEMP_RX1_TBL", \
    "12_CAL_TEMP_REF0_TBL", \
    "13_CAL_TEMP_REF1_TBL", \
    "14_CAL_VGS_C0_P0_TBL", \
    "15_CAL_VGS_C0_P1_TBL", \
    "16_CAL_VGS_C0_P2_TBL", \
    "17_CAL_VGS_C1_P0_TBL", \
    "18_CAL_VGS_C1_P1_TBL", \
    "19_CAL_VGS_C1_P2_TBL", \
    "20_CAL_PWR_VDS_TBL", \
    "21_CAL_PWR_VGS_C0_P0_TBL", \
    "22_CAL_PWR_VGS_C0_P1_TBL", \
    "23_CAL_PWR_VGS_C0_P2_TBL", \
    "24_CAL_PWR_VGS_C1_P0_TBL", \
    "25_CAL_PWR_VGS_C1_P1_TBL", \
    "26_CAL_PWR_VGS_C1_P2_TBL", \
    "27_CAL_FLTR_PARAM_TX_C0_TBL", \
    "28_CAL_FLTR_PARAM_TX_C1_TBL", \
    "29_CAL_FREQ_TX0_FB_TBL", \
    "30_CAL_FREQ_TX1_FB_TBL", \
    "31_CAL_FLTR_PARAM_RX_C0_TBL", \
    "32_CAL_FLTR_PARAM_RX_C1_TBL", \
    "33_CAL_TEMP_TX0_FB_TBL", \
    "34_CAL_TEMP_TX1_FB_TBL", \
    "35_UN_USED_TABLE_C", \
    "36_UN_USED_TABLE_D", \
    "37_CAL_VGS_C0_P3_TBL", \
    "38_CAL_VGS_C1_P3_TBL", \
    "39_CAL_PWR_VGS_C0_P3_TBL", \
    "40_CAL_PWR_VGS_C1_P3_TBL", \
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
    ABS_LNA_GAIN            = 9,       /*LNA增益绝对值, 单位0.1db 已废弃，保留兼容老版本*/
    RX1_PATH_GAIN           = 10,      /*上行通道1增益绝对值, 单位0.1db */
    RX2_PATH_GAIN           = 11,      /*上行通道2增益绝对值, 单位0.1db */
    FB_GAIN_INDEX0          = 12,
    FB_GAIN_INDEX1          = 13,
    RX_GAIN_INDEX0          = 14,
    RX_GAIN_INDEX1          = 15,

    CAL_GBL_LAST            = RX_GAIN_INDEX1,
    CAL_GLB_MAX             = 128
}CAL_GLOBAL_PARA_E;

#define GLOBAL_PARA_NAME \
{ \
    "0_CAL_K_TX1", \
    "1_CAL_B_TX1", \
    "2_CAL_K_TX2", \
    "3_CAL_B_TX2", \
    "4_CAL_K_FB1", \
    "5_CAL_B_FB1", \
    "6_CAL_K_FB2", \
    "7_CAL_B_FB2", \
    "8_CAL_LNA_GAIN", \
    "9_ABS_LNA_GAIN", \
    "10_RX1_PATH_GAIN", \
    "11_RX2_PATH_GAIN", \
    "12_FB_GAIN_INDEX0", \
    "13_FB_GAIN_INDEX1", \
    "14_RX_GAIN_INDEX0", \
    "15_RX_GAIN_INDEX1", \
}

#define CAL_TBL_MAGIC        0xA5
#define CAL_TBL_VER          1
#define CAL_TBL_MAX_LEN      1024

#define CAL_GAIN_ACCURACY    10    /*增益精度 0.1db  */
#define CAL_VGS_ACCURACY     1     /*栅压精度 1mV  */
#define CAL_K_ACCURACY       100   /*k值精度  0.01  */

#pragma pack(1)

typedef struct
{
    u32         usedlen;
    u8          buffer[CAL_TABLE_DATA_MAX_LEN];
}ru_tbl_shm_s;

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
