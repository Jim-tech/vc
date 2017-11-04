// memdb.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "string.h"
#include "windows.h"
#include "direct.h"
#include "io.h"
#include "memdb.h"
#include "table.h"


const char *g_tablename[] = CAL_TABLE_NAME; 

void print_help()
{
	printf("Usage:\n");
	printf("	memdb export srcfile dirname\n");
	printf("	memdb import dirname dstfile\n");
	printf("\n");
	return;
}

int file_copy(char *psrc, char *pdst)
{
    FILE *fp = NULL;
	u8 	  buffer[EEPROM_SIZE] = {0xFF};

	fopen_s(&fp, psrc, "rb");
	if (NULL == fp)
	{
		dbgprint("src file(%s) not exist", psrc);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	fread(buffer, 1, EEPROM_SIZE, fp);
	fclose(fp);

	fopen_s(&fp, pdst, "wb");
	if (NULL == fp)
	{
		dbgprint("dst file(%s) not exist", pdst);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	fwrite(buffer, 1, EEPROM_SIZE, fp);
	fclose(fp);
	return 0;	
}

int memdb_export(char *filename, char *dirname)
{
	if (0 != _access(dirname, 0))
	{
		_mkdir(dirname);
	}
	
	if (0 != file_copy(filename, VIRTUAL_EEPROM))
	{
		dbgprint("copy file(%s) fail", filename);
		return -1;
	}

	if (0 != ru_init_caltbl())
	{
		dbgprint("src file(%d) invalid", filename);
		return -1;
	}

	for (int tableid = CAL_GLOBAL_PARA_TBL; tableid <=CAL_TBL_LAST; tableid++)
	{
		if (!ru_caltbl_isvalid(tableid))
		{
			continue;
		}
		
		FILE  *fp = NULL;
		char   fullpath[512] = {0};
		sprintf_s(fullpath, sizeof(fullpath), "%s\\%s.csv", dirname, g_tablename[tableid]);
		fopen_s(&fp, fullpath, "w");
		if (NULL == fp)
		{
			dbgprint("export table(%s) fail", g_tablename[tableid]);
			return -1;
		}
		
		switch (tableid)
		{
			case CAL_GLOBAL_PARA_TBL:
				fprintf(fp, "index,value\n");
				for (int i = CAL_K_TX1; i <= CAL_GBL_LAST; i++)
				{
					s32 val = -1;
					ru_read_globalpara((CAL_GLOBAL_PARA_E)i, &val);
					fprintf(fp, "%d,%d\n", i, val);
				}
				break;
				
			case CAL_FREQ_TX1_TBL:
			case CAL_FREQ_TX2_TBL:
			case CAL_FREQ_RX1_TBL:
			case CAL_FREQ_RX2_TBL:
			case CAL_FREQ_FB1_TBL:
			case CAL_FREQ_FB2_TBL:
			case CAL_TEMP_TX1_TBL:
			case CAL_TEMP_TX2_TBL:
			case CAL_TEMP_RX1_TBL:
			case CAL_TEMP_RX2_TBL:
			case CAL_TEMP_FB1_TBL:
			case CAL_TEMP_FB2_TBL:
			case CAL_VGS_C1_P1_TBL:
			case CAL_VGS_C1_P2_TBL:
			case CAL_VGS_C1_P3_TBL:
			case CAL_VGS_C2_P1_TBL:
			case CAL_VGS_C2_P2_TBL:
			case CAL_VGS_C2_P3_TBL:
			case CAL_PWR_VDS_TBL:
			case CAL_PWR_VGS_C1_P1_TBL:
			case CAL_PWR_VGS_C1_P2_TBL:
			case CAL_PWR_VGS_C1_P3_TBL:
			case CAL_PWR_VGS_C2_P1_TBL:
			case CAL_PWR_VGS_C2_P2_TBL:
			case CAL_PWR_VGS_C2_P3_TBL:
			case CAL_FREQ_FB3_TBL:
			case CAL_FREQ_FB4_TBL:
			case CAL_TEMP_TX3_TBL:
			case CAL_TEMP_TX4_TBL:
			case CAL_TEMP_RX3_TBL:
			case CAL_TEMP_RX4_TBL:
			case CAL_TEMP_FB3_TBL:
			case CAL_TEMP_FB4_TBL:
			case CAL_VGS_C3_P1_TBL:
			case CAL_VGS_C3_P2_TBL:
			case CAL_VGS_C3_P3_TBL:
			case CAL_VGS_C4_P1_TBL:
			case CAL_VGS_C4_P2_TBL:
			case CAL_VGS_C4_P3_TBL:
				{
					calsave_s  sttableval = {0};
					if (0 != ru_read_caltbl(tableid, &sttableval))
					{
						dbgprint("read table(%s) fail", g_tablename[tableid]);
					}
					else
					{
						fprintf(fp, "start,step,count,base\n");
						fprintf(fp, "%d,%d,%d,%d\n", sttableval.cal_start, sttableval.cal_step, sttableval.recordcnt, sttableval.value_base);
						fprintf(fp, "key,value\n");

						cal_delta_s *pdata = (cal_delta_s *)(sttableval.data);
						for (int i = 0; i < sttableval.recordcnt; i++)
						{
							fprintf(fp, "%d,%d\n", sttableval.cal_start + i * sttableval.cal_step, pdata[i].delta);
						}
					}
				}
				break;

			case CAL_FLTR_PARAM_C1_TBL:
			case CAL_FLTR_PARAM_C2_TBL:
				{
					calsave_s  sttableval = {0};
					if (0 != ru_read_caltbl(tableid, &sttableval))
					{
						dbgprint("read table(%s) fail", g_tablename[tableid]);
					}
					else
					{
						fprintf(fp, "start,step,count\n");
						fprintf(fp, "%d,%d,%d\n", sttableval.cal_start, sttableval.cal_step, sttableval.recordcnt);
						fprintf(fp, "key,delay,attenuation\n");

						filter_param_s *pdata = (filter_param_s *)(sttableval.data);
						for (int i = 0; i < sttableval.recordcnt; i++)
						{
							fprintf(fp, "%d,%d,%d\n", sttableval.cal_start + i * sttableval.cal_step, pdata[i].delay, pdata[i].attenu);
						}
					}	
				}
				break;

			default:
				break;
		}

		fclose(fp);
	}

	return 0;
}

int memdb_import(char *dirname, char *filename)
{
	if (0 != _access(dirname, 0))
	{
		dbgprint("dir(%s) not exist", dirname);
		return -1;
	}

	if (0 != ru_init_caltbl())
	{
		dbgprint("src file(%d) invalid", filename);
		return -1;
	}

	for (int tableid = CAL_GLOBAL_PARA_TBL; tableid <=CAL_TBL_LAST; tableid++)
	{
		char   fullpath[512] = {0};
		sprintf_s(fullpath, sizeof(fullpath), "%s\\%s.csv", dirname, g_tablename[tableid]);

		FILE  *fp = NULL;
		fopen_s(&fp, fullpath, "r");
		if (NULL == fp)
		{
			dbgprint("table(%s) not exist", g_tablename[tableid]);
			continue;
		}
		
		switch (tableid)
		{
			case CAL_GLOBAL_PARA_TBL:
				{
					int   val = 0;
					int   row = 0;
					int   index = 0;
					char  szline[128] = {0}; 

					//第一行title
					fgets(szline, sizeof(szline), fp);
					row++;
					while (fgets(szline, sizeof(szline), fp))
					{
						row++;
						if (2 != sscanf_s(szline, "%d,%d", &index, &val))
						{
							dbgprint("parse table(%s) fail, row=%d", g_tablename[tableid], row);
							fclose(fp);
							return -1;
						}

						ru_save_globalpara((CAL_GLOBAL_PARA_E)index, val);
					}
				}
				break;
				
			case CAL_FREQ_TX1_TBL:
			case CAL_FREQ_TX2_TBL:
			case CAL_FREQ_RX1_TBL:
			case CAL_FREQ_RX2_TBL:
			case CAL_FREQ_FB1_TBL:
			case CAL_FREQ_FB2_TBL:
			case CAL_TEMP_TX1_TBL:
			case CAL_TEMP_TX2_TBL:
			case CAL_TEMP_RX1_TBL:
			case CAL_TEMP_RX2_TBL:
			case CAL_TEMP_FB1_TBL:
			case CAL_TEMP_FB2_TBL:
			case CAL_VGS_C1_P1_TBL:
			case CAL_VGS_C1_P2_TBL:
			case CAL_VGS_C1_P3_TBL:
			case CAL_VGS_C2_P1_TBL:
			case CAL_VGS_C2_P2_TBL:
			case CAL_VGS_C2_P3_TBL:
			case CAL_PWR_VDS_TBL:
			case CAL_PWR_VGS_C1_P1_TBL:
			case CAL_PWR_VGS_C1_P2_TBL:
			case CAL_PWR_VGS_C1_P3_TBL:
			case CAL_PWR_VGS_C2_P1_TBL:
			case CAL_PWR_VGS_C2_P2_TBL:
			case CAL_PWR_VGS_C2_P3_TBL:
			case CAL_FREQ_FB3_TBL:
			case CAL_FREQ_FB4_TBL:
			case CAL_TEMP_TX3_TBL:
			case CAL_TEMP_TX4_TBL:
			case CAL_TEMP_RX3_TBL:
			case CAL_TEMP_RX4_TBL:
			case CAL_TEMP_FB3_TBL:
			case CAL_TEMP_FB4_TBL:
			case CAL_VGS_C3_P1_TBL:
			case CAL_VGS_C3_P2_TBL:
			case CAL_VGS_C3_P3_TBL:
			case CAL_VGS_C4_P1_TBL:
			case CAL_VGS_C4_P2_TBL:
			case CAL_VGS_C4_P3_TBL:
				{
					int   	   row = 0;
					calsave_s  sttableval = {0};
					char       szline[128] = {0}; 

					//第一行title
					fgets(szline, sizeof(szline), fp);
					row++;

					//第二行start,step,count
					fgets(szline, sizeof(szline), fp);
					row++;

					int value[4] = {0};
					if (4 != sscanf_s(szline, "%d,%d,%d,%d", &value[0], &value[1], &value[2], &value[3]))
					{
						dbgprint("parse table(%s) fail, row=%d", g_tablename[tableid], row);
						fclose(fp);
						return -1;
					}
					sttableval.cal_start = value[0];
					sttableval.cal_step = (s16)value[1];
					sttableval.recordcnt = (u8)value[2];
					sttableval.value_base = (s16)value[3];
					sttableval.recordlen = sizeof(cal_delta_s);
					sttableval.version = CAL_TBL_VER;

					//第3行title
					fgets(szline, sizeof(szline), fp);
					row++;

					cal_delta_s *pdata = (cal_delta_s *)(sttableval.data);
					int index = 0;
					while (fgets(szline, sizeof(szline), fp))
					{
						row++;
						if (2 != sscanf_s(szline, "%d,%d", &value[0], &value[1]))
						{
							dbgprint("parse table(%s) fail, row=%d", g_tablename[tableid], row);
							fclose(fp);
							return -1;
						}

						pdata[index++].delta = (s16)value[1];
					}

					ru_save_caldata(tableid, &sttableval);
				}
				break;

			case CAL_FLTR_PARAM_C1_TBL:
			case CAL_FLTR_PARAM_C2_TBL:
				{
					int   	   row = 0;
					calsave_s  sttableval = {0};
					char       szline[128] = {0}; 

					//第一行title
					fgets(szline, sizeof(szline), fp);
					row++;

					//第二行start,step,count
					fgets(szline, sizeof(szline), fp);
					row++;

					int value[3] = {0};
					if (3 != sscanf_s(szline, "%d,%d,%d", &value[0], &value[1], &value[2]))
					{
						dbgprint("parse table(%s) fail, row=%d", g_tablename[tableid], row);
						fclose(fp);
						return -1;
					}

					sttableval.cal_start = value[0];
					sttableval.cal_step = (s16)value[1];
					sttableval.recordcnt = (u8)value[2];
					sttableval.recordlen = sizeof(filter_param_s);
					sttableval.version = CAL_TBL_VER;

					//第3行title
					fgets(szline, sizeof(szline), fp);
					row++;

					filter_param_s *pdata = (filter_param_s *)(sttableval.data);
					int index = 0;
					while (fgets(szline, sizeof(szline), fp))
					{
						row++;
						if (3 != sscanf_s(szline, "%d,%d,%d", &value[0], &value[1], &value[2]))
						{
							dbgprint("parse table(%s) fail, row=%d", g_tablename[tableid], row);
							fclose(fp);
							return -1;
						}

						pdata[index].delay = (s8)value[1];
						pdata[index].attenu = (s8)value[2];
						index++;
					}

					ru_save_caldata(tableid, &sttableval);
				}
				break;

			default:
				break;
		}

		fclose(fp);
	}
	
	if (0 != file_copy(VIRTUAL_EEPROM, filename))
	{
		dbgprint("copy file(%s) fail", filename);
		return -1;
	}	
	
	return 0;
}

int init_veeprom()
{
    FILE *fp = NULL;

	fopen_s(&fp, VIRTUAL_EEPROM, "w");
	if (NULL == fp)
	{
		dbgprint("virtual eeprom not exist");
		return -1;
	}

	u8 buffer[EEPROM_SIZE] = {0xFF};
	memset(buffer, 0xFF, sizeof(buffer));
	fseek(fp, 0, SEEK_SET);
	fwrite(buffer, 1, EEPROM_SIZE, fp);

	fclose(fp);
	return 0;
}

int _tmain(int argc, TCHAR* argv[])
{
	if (4 != argc)
	{
		print_help();
		return 0;
	}

	if (0 != init_veeprom())
	{
		dbgprint("init virtual eeprom fail!");
		return -1;
	}
	
	if (0 == strcmp(argv[1], "export"))
	{
		if(0 != memdb_export(argv[2], argv[3]))
		{
			dbgprint("export file(%s) to dir(%s) fail", argv[2], argv[3]);
			return -1;
		}
		else
		{
			dbgprint("export successfully!");
			return 0;
		}
	}

	if (0 == strcmp(argv[1], "import"))
	{
		if(0 != memdb_import(argv[2], argv[3]))
		{
			dbgprint("import dir(%s) to file(%s) fail", argv[2], argv[3]);
			return -1;
		}
		else
		{
			dbgprint("import successfully!");
			return 0;
		}		
	}

	print_help();
	return 0;
}

