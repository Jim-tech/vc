#ifndef __SSH_H__
#define __SSH_H__

#define MAX_SSH_CMD_LEN   8192

typedef enum
{
	ERR_WSA_START = 1,
	ERR_LIBSSH_INIT = 2,
	ERR_CONN_FAIL = 3,
	ERR_SSH_SESSION = 4,
	ERR_SSH_SHAKE = 5,
	ERR_SSH_KNOWNHOST = 6,
	ERR_SSH_FINGERPRINT = 7,
	ERR_SSH_CHANNEL = 8,
	ERR_SSH_AUTH = 9,
	ERR_SSH_PTY = 10,
	ERR_THREAD_FAIL = 11,
	ERR_SSH_NOT_READY = 12,
}SSH_ERR_E;

int  ssh_sessioninit(char *phostname, char *pusername, char *ppasswd, unsigned short port, int *pCtrlID);
void ssh_sessiondeinit(int ctrlID);
int  ssh_executecmd(int ctrlID, char *pcmd, char *pretstr, int maxretlen, int timeout);
int  ssh_getfile(int ctrlID, char *pSrcpath, char *pDstpath, int timeout);
int  ssh_putfile(int ctrlID, char *pSrcpath, char *pDstpath, int timeout);

#endif
