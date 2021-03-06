#ifndef __BRU_CMD_H__
#define __BRU_CMD_H__

typedef enum
{
	plat_t2k  		= 0,
	plat_t3k  		= 1,
}platform_e;

extern int  bru_ssh_login(char *ipstr, unsigned short port, int *psession);
extern void bru_ssh_logout(int session);
extern int bru_ssh_uploadfile(int session, char *psrc, char *pdst);
extern int bru_ssh_downloadfile(int session, char *psrc, char *pdst);
extern int bru_ssh_get_sn(int session, char *sn, int maxlen);
extern int bru_ssh_get_mac(int session, char *mac, int maxlen);
extern int bru_ssh_get_bootm(int session, int *p_bootm);
extern int bru_ssh_get_curr_version(int session, int bootm, char *version, int maxlen);
extern int bru_ssh_upgrade(int session);
extern int bru_ssh_checkback(int session, int bootm, char *version);
extern void bru_ssh_reboot(int session);
extern int bru_ssh_set_macaddr(int session, char *pmacstr);
extern void bru_ssh_complete(int session);
extern void bru_ssh_led_slowflash(int session);
extern void bru_ssh_ledfastflash(int session);



#endif /* __BRU_CMD_H__ */
