#include "stdafx.h"
#include "string.h"
#include "windows.h"
#include "memdb.h"
#include "table.h"


static u8                 g_caldatabuffer[CAL_TABLE_DATA_MAX_LEN] = {0};
static caltbl_desc_s     *g_caltbldesc = (caltbl_desc_s *)g_caldatabuffer;
static cal_global_para_s *g_calglobalpara = NULL;
static u16                g_caltblend = 0;

int eeprom_ru_read(int addr, int length, unsigned char *pdata)
{
    if ((addr < EEPROM_CAL_TABLE_BASE_ADDR) || (addr + length > EEPROM_CAL_TABLE_BASE_ADDR + 10*1024))
    {
        dbgprint("addr=0x%x len=0x%x not allowed to operate by this api!",addr,length);
        return -1;
    }
    
    FILE *fp = NULL;

	fopen_s(&fp, VIRTUAL_EEPROM, "rb");
	if (NULL == fp)
	{
		dbgprint("virtual eeprom not exist");
		return -1;
	}

	fseek(fp, addr, SEEK_SET);
	fread(pdata, 1, length, fp);

	fclose(fp);
	return 0;
}

int eeprom_ru_write(int addr, int length, unsigned char *pdata)
{
    if ((addr < EEPROM_CAL_TABLE_BASE_ADDR) || (addr + length > EEPROM_CAL_TABLE_BASE_ADDR + 10*1024))
    {
        dbgprint("addr=0x%x len=0x%x not allowed to operate by this api!",addr,length);
        return -1;
    }
    
    FILE *fp = NULL;

	fopen_s(&fp, VIRTUAL_EEPROM, "rb+");
	if (NULL == fp)
	{
		dbgprint("virtual eeprom not exist");
		return -1;
	}

	fseek(fp, addr, SEEK_SET);
	fwrite(pdata, 1, length, fp);

	fclose(fp);
	return 0;
}

u32 ru_cal_eeprom_read(int addr, int length, unsigned char *pdata)
{
    if ( (addr < EEPROM_CAL_TABLE_BASE_ADDR) || ((addr + length) > (EEPROM_CAL_TABLE_BASE_ADDR + CAL_TABLE_DATA_MAX_LEN)) )
    {
        dbgprint("read addr[0x%x] or length[0x%x] exceeds assigned range[0x%x + 0x%x]", addr, length, EEPROM_CAL_TABLE_BASE_ADDR, CAL_TABLE_DATA_MAX_LEN);
        return -1;
    }

    return eeprom_ru_read(addr, length, pdata);
}

u32 ru_cal_eeprom_write(int addr, int length, unsigned char *pdata)
{
    if ( (addr < EEPROM_CAL_TABLE_BASE_ADDR) || ((addr + length) > (EEPROM_CAL_TABLE_BASE_ADDR + CAL_TABLE_DATA_MAX_LEN)) )
    {
        dbgprint("write addr[0x%x] or length[0x%x] exceeds assigned range[0x%x + 0x%x]", addr, length, EEPROM_CAL_TABLE_BASE_ADDR, CAL_TABLE_DATA_MAX_LEN);
        return -1;
    }

    return eeprom_ru_write(addr, length, pdata);
}

u32 ru_reset_caltbl(void)
{
    u32            ret = 0;
    u32            i;
    u16            checksum = 0;
    u8            *ptbldata;
    caltbl_desc_s *pdesc = g_caltbldesc;

    g_caltblend = 0;

    /*初始化描述表  */
    memset(pdesc, 0xFF, sizeof(caltbl_desc_s) * CAL_TBL_MAX);

    pdesc[CAL_DESC_TBL].magic = CAL_TBL_MAGIC;
    pdesc[CAL_DESC_TBL].version = CAL_TBL_VER;
    pdesc[CAL_DESC_TBL].offset = 0;
    pdesc[CAL_DESC_TBL].recordcnt = CAL_TBL_MAX;
    pdesc[CAL_DESC_TBL].recordlen = sizeof(caltbl_desc_s);

    g_caltblend += sizeof(caltbl_desc_s) * CAL_TBL_MAX;

    /*初始化全局参数表  */
    pdesc[CAL_GLOBAL_PARA_TBL].magic = CAL_TBL_MAGIC;
    pdesc[CAL_GLOBAL_PARA_TBL].version = CAL_TBL_VER;
    pdesc[CAL_GLOBAL_PARA_TBL].offset = g_caltblend;
    pdesc[CAL_GLOBAL_PARA_TBL].recordcnt = CAL_GLB_MAX;
    pdesc[CAL_GLOBAL_PARA_TBL].recordlen = sizeof(cal_global_para_s);
    memset(&g_caldatabuffer[g_caltblend], 0xFF, CAL_GLB_MAX * sizeof(cal_global_para_s));
    ptbldata = &g_caldatabuffer[g_caltblend];
    checksum = 0;
    for (i = 0; i < CAL_GLB_MAX * sizeof(cal_global_para_s); i++)
    {
        checksum += ptbldata[i];
    }
    pdesc[CAL_GLOBAL_PARA_TBL].checksum = checksum;

    g_caltblend += CAL_GLB_MAX * sizeof(cal_global_para_s);

    //update desc table check sum
    checksum = 0;
    pdesc[CAL_DESC_TBL].checksum = 0;
    ptbldata = &g_caldatabuffer[0];
    for (i = 0; i < sizeof(caltbl_desc_s) * CAL_TBL_MAX; i++)
    {
        checksum += ptbldata[i];
    }
    pdesc[CAL_DESC_TBL].checksum = checksum;
    dbgprint("desc checksum=0x%x", checksum);

    /*write back  */
    ret = ru_cal_eeprom_write(EEPROM_CAL_TABLE_BASE_ADDR, g_caltblend, &g_caldatabuffer[0]);
    if (0 != ret)
    {
        dbgprint("write calibration desc to eeprom fail");
        return -1;
    }

    g_calglobalpara = (cal_global_para_s *)(g_caldatabuffer + sizeof(caltbl_desc_s) * CAL_TBL_MAX);
    return 0;
}

u32 ru_get_caltbl_offset(u8 table_id, u16 *poffset)
{
    caltbl_desc_s *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return -1;
    }

    /*如果表存在，直接返回表偏移；
      否则，从表末尾重新分配*/

    if (CAL_TBL_MAGIC == pdesc[table_id].magic)
    {
        *poffset = pdesc[table_id].offset;
        return 0;
    }
    else
    {
        dbgprint("table_id[%d] data invalid", table_id);
        return -2;
    }
}

u32 ru_get_caltbl_len(u8 table_id, u16 *plen)
{
    caltbl_desc_s *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return -1;
    }

    /*如果表存在，直接返回表总长；
      否则，认为表不存在*/

    if (CAL_TBL_MAGIC == pdesc[table_id].magic)
    {
        *plen = pdesc[table_id].recordcnt * pdesc[table_id].recordlen;
        return 0;
    }
    else
    {
        dbgprint("table_id[%d] data invalid", table_id);
        return -2;
    }
}

u32 ru_trim_caltbl(void)
{
    u32                  ret = 0;
    u8                  *pbuffer = NULL;
    u8                   table_id;
    u16                  table_offset;
    u16                  table_len;
    u16                  offset;
    caltbl_desc_s       *pdesc = g_caltbldesc;

    pbuffer = (u8 *)malloc(CAL_TABLE_DATA_MAX_LEN);
    if (NULL == pbuffer)
    {
        dbgprint("failed to alloc memory");
        return -1;
    }

    memcpy(pbuffer, g_caldatabuffer, sizeof(caltbl_desc_s) * CAL_TBL_MAX);
    offset = sizeof(caltbl_desc_s) * CAL_TBL_MAX;

    pdesc = (caltbl_desc_s *)pbuffer;
    for (table_id = CAL_GLOBAL_PARA_TBL; table_id < CAL_TBL_MAX; table_id++)
    {
        if (CAL_TBL_MAGIC != pdesc[table_id].magic)
        {
            continue;
        }

        table_offset = pdesc[table_id].offset;
        table_len = pdesc[table_id].recordcnt * pdesc[table_id].recordlen;
        memcpy(&pbuffer[offset], &g_caldatabuffer[table_offset], table_len);
        pdesc[table_id].offset = offset;
        offset += table_len;
    }

    /*write whole calibration table to eeprom  */
    ret = ru_cal_eeprom_write(EEPROM_CAL_TABLE_BASE_ADDR, offset, pbuffer);
    if (0 != ret)
    {
        dbgprint("write whole calibration data to eeprom fail");
        free(pbuffer);
        return -1;
    }

    memcpy(g_caldatabuffer, pbuffer, offset);
    g_caltblend = offset;

    free(pbuffer);
    return 0;
}

u32 ru_alloc_caltbl(u16 len, u16 *poffset)
{
    u32 ret = 0;

    if (g_caltblend + len <= CAL_TABLE_DATA_MAX_LEN)
    {
        *poffset = g_caltblend;
        g_caltblend += len;
        return 0;
    }

    dbgprint("last table end at[0x%x], need[0x%x], exceeds the maximum[0x%x], try trim the eeprom", g_caltblend, len, CAL_TABLE_DATA_MAX_LEN);

    /*空间不足时，对表空间进行碎片整理，然后重新申请一次  */
    ret = ru_trim_caltbl();
    if (0 != ret)
    {
        dbgprint("trim cal table fail");
        return -1;
    }

    if (g_caltblend + len <= CAL_TABLE_DATA_MAX_LEN)
    {
        *poffset = g_caltblend;
        g_caltblend += len;
        return 0;
    }
    else
    {
        dbgprint("not enough space, alloc[%d] left[%d]", len, CAL_TABLE_DATA_MAX_LEN - g_caltblend);
        return -2;
    }
}

u32 ru_check_caltbl_desc(void)
{
    u32            table_len;
    u16            checksum = 0;
    u16            expect_checksum = 0;
    u32            i;
    u8            *pdata = (u8 *)g_caltbldesc;
    caltbl_desc_s *pdesc = g_caltbldesc;

    if (CAL_TBL_MAGIC != pdesc[CAL_DESC_TBL].magic)
    {
        dbgprint("desc table invalid");
        return -1;
    }

    table_len = pdesc[CAL_DESC_TBL].recordcnt * pdesc[CAL_DESC_TBL].recordlen;
    if (CAL_TABLE_DATA_MAX_LEN < pdesc[CAL_DESC_TBL].offset + table_len)
    {
        dbgprint("table[%d] offset[0x%x] or len[0x%x] invalid", CAL_DESC_TBL, pdesc[CAL_DESC_TBL].offset, table_len);
        return -2;
    }

    //描述表的checksum在表内容中, 校验时，只计算其他字节的校验和. 所以计算前先置0，计算后恢复
    expect_checksum = pdesc[CAL_DESC_TBL].checksum;
    pdesc[CAL_DESC_TBL].checksum = 0;
    for (i = 0; i < table_len; i++)
    {
        checksum += pdata[i];
    }
    pdesc[CAL_DESC_TBL].checksum = expect_checksum;

    if (expect_checksum != checksum)
    {
        dbgprint("desc table checksum[0x%x] != expect[0x%x]", checksum, expect_checksum);
        return -3;
    }

    return 0;
}


u32 ru_check_caltbl(u8 table_id)
{
    u16            checksum = 0;
    u16            expect_checksum = 0;
    u32            table_len;
    u32            i;
    u8            *pdata = NULL;
    caltbl_desc_s *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return -1;
    }

    /*desc table 不在这里检查  */
    if (CAL_DESC_TBL == table_id)
    {
        return 0;
    }

    if (CAL_TBL_MAGIC != pdesc[table_id].magic)
    {
        return -2;
    }

    table_len = pdesc[table_id].recordcnt * pdesc[table_id].recordlen;
    if (CAL_TABLE_DATA_MAX_LEN < pdesc[table_id].offset + table_len)
    {
        dbgprint("table[%d] offset[0x%x] or len[0x%x] invalid", table_id, pdesc[table_id].offset, table_len);
        return -2;
    }

    expect_checksum = pdesc[table_id].checksum;
    pdata = &(g_caldatabuffer[pdesc[table_id].offset]);
    for (i = 0; i < table_len; i++)
    {
        checksum += pdata[i];
    }

    if (expect_checksum != checksum)
    {
        dbgprint("table[%d] checksum[0x%x] != expect[0x%x]", table_id, checksum, expect_checksum);
        return -3;
    }

    return 0;
}

u32 ru_set_caltbl_invalid(u8 table_id)
{
    caltbl_desc_s *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return -1;
    }

    pdesc[table_id].magic = 0xFF;
    return 0;
}

u32 ru_init_caltbl(void)
{
    u32            ret = 0;
    u8             table_id = 0;
    u16            offset;
    u16            len;
    caltbl_desc_s *pdesc = g_caltbldesc;

    memset(g_caldatabuffer, 0xff, sizeof(g_caldatabuffer));

    ret = ru_cal_eeprom_read(EEPROM_CAL_TABLE_BASE_ADDR, CAL_TABLE_DATA_MAX_LEN, g_caldatabuffer);
    if (0 != ret)
    {
        dbgprint("read calibration data from eeprom fail");
        return -1;
    }

    #if  0
    for (i = 0; i < CAL_TABLE_DATA_MAX_LEN; i++)
    {
        if (0 == (i % 16))
        {
            printf("\n%04x:", i+0x7800);
        }

        printf(" %02x", g_caldatabuffer[i]);
    }
    printf("\n");
    #endif /* #if 0 */

    if (0 != ru_check_caltbl_desc())
    {
        return ru_reset_caltbl();
    }

    g_caltblend = sizeof(caltbl_desc_s) * CAL_TBL_MAX;
    for (table_id = CAL_GLOBAL_PARA_TBL; table_id < CAL_TBL_MAX; table_id++)
    {
    	
		//为兼容已发货产品的前向/反馈/反向温度补偿数据不正确而处理
#if 0
		if ((table_id >= CAL_TEMP_TX1_TBL && table_id <= CAL_TEMP_FB2_TBL)
			&& pdesc[table_id].version == 1)
		{
			ru_set_caltbl_invalid(table_id);
		}
#endif			
	
        if (0 != ru_check_caltbl(table_id))
        {
            ru_set_caltbl_invalid(table_id);
        }
        else
        {
            offset = pdesc[table_id].offset;
            len = pdesc[table_id].recordcnt * pdesc[table_id].recordlen;
            if (g_caltblend < offset + len)
            {
                g_caltblend = offset + len;
            }
        }
    }

    g_calglobalpara = (cal_global_para_s *)(g_caldatabuffer + sizeof(caltbl_desc_s) * CAL_TBL_MAX);

    return 0;
}

u32 ru_update_caltbl(u8 table_id)
{
    u32            ret = 0;
    u16            checksum = 0;
    u16            len;
    u16            offset;
    u16            i;
    caltbl_desc_s *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return -1;
    }

    len = pdesc[table_id].recordcnt * pdesc[table_id].recordlen;
    offset = pdesc[table_id].offset;

    ret = ru_cal_eeprom_write(EEPROM_CAL_TABLE_BASE_ADDR + offset, len, &g_caldatabuffer[offset]);
    if (0 != ret)
    {
        dbgprint("write table[%d] data to eeprom fail", table_id);
        return -1;
    }

    //update check sum
    checksum = 0;
    for (i = 0; i < len; i++)
    {
        checksum += g_caldatabuffer[offset + i];
    }
    pdesc[table_id].checksum = checksum;

    dbgprint("table[%d] offset[0x%x] len[0x%x] checksum[0x%x]", table_id, offset, len, checksum);

    offset = sizeof(caltbl_desc_s) * table_id;
    ret = ru_cal_eeprom_write(EEPROM_CAL_TABLE_BASE_ADDR + offset, sizeof(caltbl_desc_s), (u8 *)&pdesc[table_id]);
    if (0 != ret)
    {
        dbgprint("write table[%d] desc to eeprom fail", table_id);
        return -1;
    }

    pdesc[table_id].magic = CAL_TBL_MAGIC;

    //update check sum of desc table
    offset = 0;
    len = sizeof(caltbl_desc_s) * CAL_TBL_MAX;
    pdesc[CAL_DESC_TBL].checksum = 0;
    checksum = 0;
    for (i = 0; i < len; i++)
    {
        checksum += g_caldatabuffer[offset + i];
    }
    pdesc[CAL_DESC_TBL].checksum = checksum;

    dbgprint("table[%d] offset[0x%x] len[0x%x] checksum[0x%x]", CAL_DESC_TBL, offset, len, checksum);
    ret = ru_cal_eeprom_write(EEPROM_CAL_TABLE_BASE_ADDR + offset, sizeof(caltbl_desc_s), (u8 *)&pdesc[CAL_DESC_TBL]);
    if (0 != ret)
    {
        dbgprint("write desc table to eeprom fail");
        return -1;
    }

    return 0;
}

u32 ru_save_caldata(u8 table_id, calsave_s *ppara)
{
    u32            ret = 0;
    u16            len;
    u16            oldlen;
    u16            offset;
    caltbl_desc_s  stdesc = {0};
    caltbl_desc_s *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return -1;
    }

    memset(&stdesc, 0xFF, sizeof(caltbl_desc_s));
    stdesc.magic = CAL_TBL_MAGIC;
    stdesc.version = ppara->version;
    stdesc.recordcnt = ppara->recordcnt;
    stdesc.recordlen = ppara->recordlen;
    stdesc.cal_start = ppara->cal_start;
    stdesc.cal_step = ppara->cal_step;
    stdesc.value_base = ppara->value_base;

    len = ppara->recordcnt * ppara->recordlen;
    if (CAL_TBL_MAX_LEN < len)
    {
        dbgprint("table_id[%d] length[%d] exceeds the maximum[%d]", table_id, len, CAL_TBL_MAX_LEN);
        return -1;
    }

    ret = ru_get_caltbl_len(table_id, &oldlen);
    if (0 == ret)
    {
        ret = ru_get_caltbl_offset(table_id, &offset);
        if (0 != ret)
        {
            dbgprint("table[%d] get offset fail", table_id);
            return ret;
        }

        if (len <= oldlen)
        {
            stdesc.offset = offset;
            memcpy(&g_caldatabuffer[offset], ppara->data, len);
            memcpy(&(pdesc[table_id]), &stdesc, sizeof(caltbl_desc_s));
            return ru_update_caltbl(table_id);
        }
    }

    /*其他情况，需要重新申请空间  */
    ret = ru_alloc_caltbl(len, &offset);
    if (0 != ret)
    {
        return ret;
    }

    dbgprint("table[%d] alloc len=0x%x offset=0x%x ", table_id, len, offset);

    stdesc.offset = offset;
    memcpy(&g_caldatabuffer[offset], ppara->data, len);
    memcpy(&(pdesc[table_id]), &stdesc, sizeof(caltbl_desc_s));
    return ru_update_caltbl(table_id);
}

u32 ru_save_globalpara(CAL_GLOBAL_PARA_E type, s32 value)
{
    if (CAL_GLB_MAX <= type)
    {
        dbgprint("invalid para type[%d]", type);
        return -1;
    }

    g_calglobalpara[type].para_val = value;
    return ru_update_caltbl(CAL_GLOBAL_PARA_TBL);
}

u32 ru_read_caltbl(u8 table_id, calsave_s *pcaltbl)
{
    u16             offset = 0;
    u16             length = 0;
    caltbl_desc_s  *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return -1;
    }

    if (CAL_TBL_MAGIC != pdesc[table_id].magic)
    {
        //dbgprint("table_id[%d] invalid", table_id);
        return -2;
    }

    pcaltbl->version = pdesc[table_id].version;
    pcaltbl->recordcnt = pdesc[table_id].recordcnt;
    pcaltbl->recordlen = pdesc[table_id].recordlen;
    pcaltbl->cal_start = pdesc[table_id].cal_start;
    pcaltbl->cal_step = pdesc[table_id].cal_step;
    pcaltbl->value_base = pdesc[table_id].value_base;

    offset = pdesc[table_id].offset;
    length = pdesc[table_id].recordcnt * pdesc[table_id].recordlen;
    memcpy(&(pcaltbl->data[0]), &g_caldatabuffer[offset], length);
    return 0;
}

u32 ru_read_globalpara(CAL_GLOBAL_PARA_E type, s32 *pvalue)
{
    if (CAL_GLB_MAX <= type)
    {
        dbgprint("invalid para type[%d]", type);
        return -1;
    }

    *pvalue = g_calglobalpara[type].para_val;
    return 0;
}

bool ru_caltbl_isvalid(u8 table_id)
{
    caltbl_desc_s  *pdesc = g_caltbldesc;

    if (CAL_TBL_MAX <= table_id)
    {
        dbgprint("table_id[%d] exceeds the maximum[%d]", table_id, CAL_TBL_MAX);
        return false;
    }

    if (CAL_TBL_MAGIC != pdesc[table_id].magic)
    {
        //dbgprint("table_id[%d] invalid", table_id);
        return false;
    }

    return true;
}

int ru_get_caltbl_value(u8 table_id, int key)
{
    calsave_s  desc;
	if (! ru_caltbl_isvalid(table_id)) return 0;
	ru_read_caltbl(table_id, &desc);
    cal_delta_s *data = (cal_delta_s *)&(desc.data[0]);

    int x0, x1, y0, y1, x = key;
    int i;
    double y = 0;
	
    if(desc.cal_step >= 0)     {
        if (x <= desc.cal_start) return data[0].delta + desc.value_base;
        if (x >= desc.cal_start + desc.cal_step * (desc.recordcnt -1)) 
			return data[desc.recordcnt - 1].delta + desc.value_base;

        x0 = x1 = desc.cal_start;
        for (i= 0; i < desc.recordcnt - 1; i++)
        {
            x0 = x1;
            x1 = x1 + desc.cal_step;

            if (x >= x0 && x <= x1)
            {
                 y0 = data[i].delta, y1 = data[i+1].delta;
                //printf("x:%d, x0:%d, x1:%d, y0:%d, y1:%d, y:%f\n", x, x0, x1, y0, y1, y);

                y = y0 + (y1 - y0) * (x - x0) / (x1-x0) + desc.value_base;
                break;
            }
        }
    }else  {
        if (x >= desc.cal_start) return data[0].delta + desc.value_base;
        if (x <= desc.cal_start + desc.cal_step * (desc.recordcnt - 1)) 
			return data[desc.recordcnt - 1].delta + desc.value_base;

        x0 = x1 = desc.cal_start;
        for (i= desc.recordcnt - 1; i > 0; i--)
        {
            x0 = x1;
            x1 = x1 + desc.cal_step;

            if (x <= x0 && x >= x1)
            {
                y0 = data[i].delta, y1 = data[i-1].delta;
                //printf("x:%d, x0:%d, x1:%d, y0:%d, y1:%d, y:%f\n", x, x0, x1, y0, y1, y);

                y = y0 + (y1 - y0) * (x - x0) / (x1-x0) + desc.value_base;
                break;
            }
        }
    }


    return (int)y;
}
