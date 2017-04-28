#ifndef __BRU_CMD_H__
#define __BRU_CMD_H__


typedef struct iplist
{
	DWORD               ipaddr;
    char                macaddr[6];
	char                szsn[19+1];
	char                szmac[12+1];
    char                szversion[128+1];
    int                 state;
    int                 waittime;
	bool                runnning;
	bool                used;
}IPList_S;


extern int  bru_ssh_login(DWORD ipaddr, int *psession);
extern void bru_ssh_logout(int session);
extern int bru_ssh_uploadfile(int session, char *psrc, char *pdst);
extern int bru_ssh_get_sn(int session, char *sn, int maxlen);
extern int bru_ssh_get_mac(int session, char *mac, int maxlen);
extern int bru_ssh_get_bootm(int session, int *p_bootm);
extern int bru_ssh_get_curr_version(int session, int bootm, char *version, int maxlen);
extern int bru_ssh_upgrade(int session);
extern int bru_ssh_checkback(int session, int bootm, char *version);
extern void bru_ssh_reboot(int session);



#endif /* __BRU_CMD_H__ */
