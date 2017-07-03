// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <tchar.h>
#include "stdio.h"
#include "stdlib.h"
#include <time.h>

#include <WS2tcpip.h>
#include <winsock2.h>
#include "libssh2.h"
#include "ssh.h"

#define  MAX_SCP_MTU       1400


#define  MAX_SSH_CTRL_CNT  16

typedef struct sshsession
{
	bool      IsUsed;	
	bool      IsReady;
	char      szInput[MAX_SSH_CMD_LEN];   //输入队列
	char      szOutput[MAX_SSH_CMD_LEN];  //输出队列
	HANDLE    hReqEvent;         //控制输入输出时序
	HANDLE    hReplyEvent;       //控制输入输出时序
	HANDLE    hInputMutex;       //主要是保护输入队列，内部使用
	HANDLE    hOutputMutex;      //主要是保护输出队列，内部使用
	HANDLE    hSshThread;
    int       socket;
	int       timeout;           //毫秒
	int       closing;
	LIBSSH2_SESSION *pSshSession;
	LIBSSH2_CHANNEL *pSshChannel;
}SSH_CTRL_S;

SSH_CTRL_S g_astSshCtrl[MAX_SSH_CTRL_CNT];

//定义特殊的命令，用于标记是scp的操作
#define SCP_GET_MAGIC_STRING  "[*!scp-get!*]"
#define SCP_PUT_MAGIC_STRING  "[*!scp-put!*]"

extern FILE *fplog;
#define dbgprint(...) \
    do\
    {\
		SYSTEMTIME __sys;\
		GetLocalTime(&__sys);\
		if (NULL != fplog) \
		{\
            fprintf(fplog, "[%04d-%02d-%02d %02d:%02d:%02d][%s][%d][0x%08x]", __sys.wYear, __sys.wMonth, __sys.wDay, \
            					__sys.wHour, __sys.wMinute, __sys.wSecond, __FUNCTION__, __LINE__, GetCurrentThreadId());\
    		fprintf(fplog, __VA_ARGS__);\
    		fprintf(fplog, "\n");\
    		fflush(fplog);\
		}\
		printf("[%04d-%02d-%02d %02d:%02d:%02d][%s][%d][0x%08x]", __sys.wYear, __sys.wMonth, __sys.wDay, \
        					__sys.wHour, __sys.wMinute, __sys.wSecond, __FUNCTION__, __LINE__, GetCurrentThreadId());\
        printf(__VA_ARGS__);\
        printf("\n");\
        fflush(stdout);\
        fflush(stderr);\
     }while(0)

void ssh_readinput(SSH_CTRL_S *pCtrl, char *pcmdstr, int maxbufferlen)
{
	WaitForSingleObject(pCtrl->hInputMutex,15000);
	pcmdstr[0] = '\0';
	strncpy_s(pcmdstr, maxbufferlen-1, pCtrl->szInput, strlen(pCtrl->szInput)); 
	memset(pCtrl->szInput, 0, sizeof(pCtrl->szInput));
	ReleaseMutex(pCtrl->hInputMutex);
}

void ssh_writeinput(SSH_CTRL_S *pCtrl, char *pcmdstr)
{
	WaitForSingleObject(pCtrl->hInputMutex,15000);
	pCtrl->szInput[0] = '\0';
	strncpy_s(pCtrl->szInput, sizeof(pCtrl->szInput)-1, pcmdstr, strlen(pcmdstr)); 
	ReleaseMutex(pCtrl->hInputMutex);
}

void ssh_readoutput(SSH_CTRL_S *pCtrl, char *pcmdstr, int maxbufferlen)
{
	WaitForSingleObject(pCtrl->hOutputMutex,15000);

	memset(pcmdstr, 0, maxbufferlen);

	//过滤输出字符中的控制字符
	bool ctrlstate = false;
	int  ansilen = 0;
	for (unsigned int i = 0; i < strlen(pCtrl->szOutput) && (ansilen < maxbufferlen); i++)
	{
		if (ctrlstate)
		{
			if (pCtrl->szOutput[i - 1] == 0x1b)
			{
				continue;
			}
			
			if (pCtrl->szOutput[i] > 64)
			{
				ctrlstate = false;
				continue;
			}
			else
			{
				//do nothing
			}
		}
		else
		{
			if (pCtrl->szOutput[i] == 0x1b)
			{
				ctrlstate = true;
				continue;
			}
			else
			{
				pcmdstr[ansilen++] = pCtrl->szOutput[i];
			}
		}
	}
	pcmdstr[maxbufferlen-1] = '\0';
	
	memset(pCtrl->szOutput, 0, sizeof(pCtrl->szOutput));
	ReleaseMutex(pCtrl->hOutputMutex);	
}

void ssh_writeoutput(SSH_CTRL_S *pCtrl, char *pcmdstr)
{
	WaitForSingleObject(pCtrl->hOutputMutex,15000);
	pCtrl->szOutput[0] = '\0';
	strncpy_s(pCtrl->szOutput, sizeof(pCtrl->szOutput)-1, pcmdstr, strlen(pcmdstr)); 
	ReleaseMutex(pCtrl->hOutputMutex);
}


void ssh_closesession(SSH_CTRL_S *pCtrl)
{
	int timecnt = 0;
	
	//TerminateThread(pCtrl->hSshThread, 0);
	pCtrl->closing = true;
	while (pCtrl->closing)
	{
		Sleep(100);
		timecnt += 100;
		if (timecnt > 10000)
		{
			pCtrl->closing = false;
			TerminateThread(pCtrl->hSshThread, 0);
			break;
		}
	}
}

static void kbd_callback(const char *name, int name_len,
                         const char *instruction, int instruction_len,
                         int num_prompts,
                         const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                         LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                         void **abstract)
{
    (void)name;
    (void)name_len;
    (void)instruction;
    (void)instruction_len;
    (void)prompts;
    (void)abstract;
} /* kbd_callback */

static int sock_wait4read(int socket_fd, LIBSSH2_SESSION *session, int millisec)
{
    struct timeval timeout;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;

	timeout.tv_sec = millisec / 1000;
	timeout.tv_usec = (millisec % 1000) * 1000;

    FD_ZERO(&fd);
    FD_SET(socket_fd, &fd);
    readfd = &fd;

    return select(socket_fd + 1, readfd, writefd, NULL, &timeout);;
}

static int sock_wait4write(int socket_fd, LIBSSH2_SESSION *session, int millisec)
{
    struct timeval timeout;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;

	timeout.tv_sec = millisec / 1000;
	timeout.tv_usec = (millisec % 1000) * 1000;

    FD_ZERO(&fd);
    FD_SET(socket_fd, &fd);
    writefd = &fd;

    return select(socket_fd + 1, readfd, writefd, NULL, &timeout);;
}

static int sock_wait4scp(int socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    fd_set  fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
	int     dir;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

    FD_ZERO(&fd);
    FD_SET(socket_fd, &fd);
	
    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);

    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
        readfd = &fd;

    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
        writefd = &fd;

    return select(socket_fd + 1, readfd, writefd, NULL, &timeout);;
}

int ssh_session_getfile(int sock, LIBSSH2_SESSION *session, char *pInput)
{
	int    rc;
	int    start;
	int    timecost;
	
	char *pSrc = strstr(pInput, "|");
	if (NULL == pSrc)
	{
        dbgprint("input para(%s) invalid", pInput);
		return -1;
	}
	else
	{
		*pSrc = '\0';
		pSrc += strlen("|");
	}

	char *pDst = strstr(pSrc, "|");
	if (NULL == pDst)
	{
		dbgprint("input para(%s) invalid", pInput);
        return -1;
	}
	else
	{
		*pDst = '\0';
		pDst += strlen(":");
	}

	dbgprint("scp file (%s) --> (%s)", pSrc, pDst);

	LIBSSH2_CHANNEL *channel;
	struct stat fileinfo;
	off_t got=0;

	do
	{
		channel = libssh2_scp_recv(session, pSrc, &fileinfo);
	    if (!channel) {
			if(libssh2_session_last_errno(session) != LIBSSH2_ERROR_EAGAIN) {
				char *err_msg;
				libssh2_session_last_error(session, &err_msg, NULL, 0);
				dbgprint("failed to get file(%s), err=%s", pSrc, err_msg);
				return -1;
			}
	    }
	}while (!channel);

	FILE *fp = NULL;
	rc = fopen_s(&fp, pDst, "wb+");
	if (NULL == fp || rc != 0)
	{
		dbgprint("failed to create file(%s)", pDst);
		libssh2_channel_free(channel);
        return -1;
	}

	dbgprint("(%s) total size %lu bytes", pSrc, fileinfo.st_size);
	dbgprint("");
	start = (int)time(NULL);
	while (got < fileinfo.st_size) 
	{
		char mem[MAX_SCP_MTU];
		int  amount = sizeof(mem);

		if((fileinfo.st_size - got) < amount) 
		{
			amount = fileinfo.st_size -got;
		}

		rc = libssh2_channel_read(channel, mem, amount);
		if (rc > 0) 
		{
			//写文件
			fwrite(mem, 1, rc, fp);
			got += rc;
			//dbgprint("(%s) read %d/%lu bytes", pSrc, got, fileinfo.st_size);
			printf("\r(%s) get %lu/%lu bytes", pSrc, got, fileinfo.st_size);
		}
		else if (LIBSSH2_ERROR_EAGAIN == rc)
		{
			sock_wait4scp(sock, session);
		}
		else
		{
			dbgprint("(%s), unexpected error occured", pSrc);
		    libssh2_channel_free(channel);
			fclose(fp);
			return -2;
		}
	}
	timecost = (int)time(NULL) - start;
	dbgprint("");
	if (timecost > 0)
	{
		dbgprint("(%s) download done, speed %d B/s", pSrc, fileinfo.st_size/timecost);
	}
	
    libssh2_channel_free(channel);
	fclose(fp);
	return 0;	
}

int ssh_session_putfile(int sock, LIBSSH2_SESSION *session, char *pInput)
{
	int    rc;
	int    start;
	int    timecost;
	
	char *pSrc = strstr(pInput, "|");
	if (NULL == pSrc)
	{
        dbgprint("input para(%s) invalid", pInput);
		return -1;
	}
	else
	{
		*pSrc = '\0';
		pSrc += strlen("|");
	}

	char *pDst = strstr(pSrc, "|");
	if (NULL == pDst)
	{
        dbgprint("input para(%s) invalid", pInput);
        return -1;
	}
	else
	{
		*pDst = '\0';
		pDst += strlen("|");
	}

	dbgprint("scp file (%s) --> (%s)", pSrc, pDst);

	LIBSSH2_CHANNEL *channel;
	struct stat fileinfo;
	stat(pSrc, &fileinfo);

    /* Send a file via scp. The mode parameter must only have permissions! */
	do
	{
    	channel = libssh2_scp_send(session, pDst, fileinfo.st_mode & 0777, (unsigned long)fileinfo.st_size);
        if ((!channel) && (libssh2_session_last_errno(session) != LIBSSH2_ERROR_EAGAIN)) 
		{
            char *err_msg;
            libssh2_session_last_error(session, &err_msg, NULL, 0);
            dbgprint("failed to put file[%s], err=%s st_mode=0x%x st_size=0x%x", pDst, err_msg, fileinfo.st_mode, fileinfo.st_size);
			return -1;
        }
	} while(!channel);
	
	FILE *fp = NULL;
	rc = fopen_s(&fp, pSrc, "rb");
	if (NULL == fp || rc != 0)
	{
		dbgprint("open file fail, [%s]", pSrc);
		libssh2_channel_free(channel);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	dbgprint("(%s) total %d bytes", pSrc, len);
	dbgprint("");
	rewind(fp);
	start = (int)time(NULL);
    do {
		char mem[MAX_SCP_MTU];
        int nread = fread(mem, 1, sizeof(mem), fp);
        if (nread <= 0) {
            break;
        }

		char *ptr = mem;
        do 
		{
            /* write the same data over and over, until error or completion */
            rc = libssh2_channel_write(channel, ptr, nread);
            if (rc > 0) 
			{
                ptr += rc;
				nread -= rc;
				//dbgprint("(%s)write %d bytes", pSrc, ptr - mem);
				printf("\r(%s) put %lu/%lu bytes", pSrc, ftell(fp), len);
            }
			else if (LIBSSH2_ERROR_EAGAIN == rc)
			{
				sock_wait4scp(sock, session);
			}
			else
			{
				dbgprint("(%s) unexpected error occured", pSrc);
			    libssh2_channel_send_eof(channel);
			    libssh2_channel_wait_eof(channel);
			    libssh2_channel_wait_closed(channel);
			    libssh2_channel_free(channel);
				fclose(fp);
				return -2;
			}
        } while (nread > 0);
    } while (1);
	timecost = (int)time(NULL) - start;
	dbgprint("");

	if (0 != timecost)
	{
		dbgprint("(%s) upload done, speed %d B/s", pSrc, len/timecost);
	}
	else
	{
		dbgprint("(%s) upload done", pSrc);
	}

    libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);
    libssh2_channel_free(channel);
	fclose(fp);
	
	return 0;
}

int ssh_session_execli(int sock, LIBSSH2_CHANNEL *channel, LIBSSH2_SESSION *session, char *pInput, char *pOutput, int timeout)
{
	int  rc;
	char buffer[0x4000];
	int  offset = 0;

	//先读清上次的
	libssh2_channel_read(channel, buffer, sizeof(buffer));
	
	do
	{
		do
		{
			rc = sock_wait4write(sock, session, 100);
			if (rc < 0)
			{
				dbgprint("failed to exec cmd[%s], wait socket fail", pInput);
				return -1;
			}
		}while(rc == 0);

		//rc = libssh2_channel_write(channel, &pInput[offset], strlen(pInput) + 1 - offset);
		rc = libssh2_channel_write(channel, &pInput[offset], strlen(pInput) - offset);
		if (rc > 0)
		{
			//dbgprint("write %d bytes", rc);
			offset += rc;
		}
	}while(((unsigned int)offset < strlen(pInput)) && (rc > 0));//while(((unsigned int)offset < strlen(pInput) + 1) && (rc > 0));

	Sleep(timeout);
	
	offset = 0;
	rc = libssh2_channel_read(channel, &buffer[offset], sizeof(buffer)-offset);
	if (rc > 0)
	{
		offset += rc;
		//dbgprint("read %d bytes", rc);
	}		

	int OutputCnt = 0;
	//回显中前一部分是输入的命令
    for(int i=0; i < rc; ++i )
    {
		if ((unsigned int)i > strlen(pInput))
		{
			pOutput[OutputCnt++] = buffer[i];
		}
		
		if (MAX_SSH_CMD_LEN-1 <= OutputCnt)
		{
			break;
		}		
    }
	
	return 0;	
}

void ssh_session(SSH_CTRL_S *pCtrl, LIBSSH2_CHANNEL *channel, int sock, LIBSSH2_SESSION *session)
{
	int rc;
	int OutputCnt = 0;
	char szCmdInput[MAX_SSH_CMD_LEN];
	char szCmdOutput[MAX_SSH_CMD_LEN];

	//读空当前的回显
	pCtrl->IsReady = true;

	dbgprint("enter ssh_session");
	while (1)
	{
		if (pCtrl->closing)
		{
			break;
		}
		
		if (WAIT_OBJECT_0 != WaitForSingleObject(pCtrl->hReqEvent, 1000))
		{
			continue;
		}
		
		memset(szCmdInput, 0, sizeof(szCmdInput));
		ssh_readinput(pCtrl, szCmdInput, sizeof(szCmdInput));

		if (szCmdInput == strstr(szCmdInput, SCP_GET_MAGIC_STRING))
		{
			rc = ssh_session_getfile(sock, session, szCmdInput);
			if (0 != rc)
			{
				dbgprint("get file fail");
			}
			
            SetEvent(pCtrl->hReplyEvent);
			continue;
		}
		else if (szCmdInput == strstr(szCmdInput, SCP_PUT_MAGIC_STRING))
		{
			rc = ssh_session_putfile(sock, session, szCmdInput);
			if (0 != rc)
			{
				dbgprint("put file fail");
			}
			
			SetEvent(pCtrl->hReplyEvent);
			continue;
		}
		else
		{
			memset(szCmdOutput, 0, sizeof(szCmdOutput));
			dbgprint("input=[%s]", szCmdInput);
			strcat_s(szCmdInput, sizeof(szCmdInput), "\n");
			rc = ssh_session_execli(sock, channel, session, szCmdInput, szCmdOutput, pCtrl->timeout);
			if (0 != rc)
			{
				dbgprint("exec cmd fail");
			}
			
			szCmdOutput[strlen(szCmdOutput)] = '\0';
			ssh_writeoutput(pCtrl, szCmdOutput);
			dbgprint("Outcmd=[%s]", szCmdOutput);
			
			SetEvent(pCtrl->hReplyEvent);	
			continue;
		}

		if (libssh2_channel_eof(channel))
		{
			break;
		}
	}

	pCtrl->closing = false;
	return;
}

DWORD WINAPI ssh_sessionthread(LPVOID lpParameter)
{
	SSH_CTRL_S *pCtrl = NULL;

	pCtrl = (SSH_CTRL_S *)lpParameter;

	//命令处理
	ssh_session(pCtrl, pCtrl->pSshChannel, pCtrl->socket, pCtrl->pSshSession);

	pCtrl->IsReady = false;
	return 0;
}

int ssh_runtimeInit()
{
	int rc;
	#if  0
	WSADATA wsadata;
	int err;
	#endif /* #if 0 */

	/*只在第一次使用的时候初始化，最后一次退出时去初始化*/
	for (int ctrlID = 0; ctrlID < MAX_SSH_CTRL_CNT; ctrlID++)
	{
		if (g_astSshCtrl[ctrlID].IsUsed)
		{
			return 0;
		}
	}
	
    #if  0
    err = WSAStartup(MAKEWORD(2,0), &wsadata);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return ERR_WSA_START;
    }
    #endif /* #if 0 */

	dbgprint("libssh2_init...");
    rc = libssh2_init(0);
    if (rc != 0) {
        dbgprint("libssh2 initialization failed (%d)", rc);
		#if  0
		WSACleanup( );
		#endif /* #if 0 */
        return ERR_LIBSSH_INIT;
    }
	dbgprint("libssh2_init...ok");

	return 0;
}

void ssh_runtimeDeInit()
{
	/*只在第一次使用的时候初始化，最后一次退出时去初始化*/
	for (int ctrlID = 0; ctrlID < MAX_SSH_CTRL_CNT; ctrlID++)
	{
		if (g_astSshCtrl[ctrlID].IsUsed)
		{
			return;
		}
	}
	
	#if  0
	WSACleanup( );
	#endif /* #if 0 */

	dbgprint("libssh2_exit...");
	libssh2_exit();
	dbgprint("libssh2_exit...ok");
}

int ssh_startsession(SSH_CTRL_S *pCtrl, char *phostname, char *pusername, char *ppasswd, unsigned short port)
{
    unsigned long hostaddr;
    int auth_pw = 0;
    struct sockaddr_in sin;
    const char *fingerprint;
    char *userauthlist;

	//hostaddr = inet_addr(phostname);
	inet_pton(AF_INET, phostname, &hostaddr);

    /* Ultra basic "connect to port 22 on localhost"
     * Your code is responsible for creating the socket establishing the
     * connection
     */
    pCtrl->socket = socket(AF_INET, SOCK_STREAM, 0);

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = hostaddr;
	dbgprint("ssh connectting...ip=%s, port=%d", phostname, port);
    if (connect(pCtrl->socket, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) 
	{
        dbgprint("failed to connect!");
        return ERR_CONN_FAIL;
    }

    /* Create a session instance and start it up. This will trade welcome
     * banners, exchange keys, and setup crypto, compression, and MAC layers
     */
    pCtrl->pSshSession = libssh2_session_init();
    if (libssh2_session_handshake(pCtrl->pSshSession, pCtrl->socket)) {
        dbgprint("Failure establishing SSH session");
		return ERR_SSH_SHAKE;
    }

    /* At this point we havn't authenticated. The first thing to do is check
     * the hostkey's fingerprint against our known hosts Your app may have it
     * hard coded, may go to a file, may present it to the user, that's your
     * call
     */
    fingerprint = libssh2_hostkey_hash(pCtrl->pSshSession, LIBSSH2_HOSTKEY_HASH_SHA1);

    /* check what authentication methods are available */
    userauthlist = libssh2_userauth_list(pCtrl->pSshSession, pusername, strlen(pusername));
    if (strstr(userauthlist, "password") != NULL) {
        auth_pw |= 1;
    }
    if (strstr(userauthlist, "keyboard-interactive") != NULL) {
        auth_pw |= 2;
    }
    if (strstr(userauthlist, "publickey") != NULL) {
        auth_pw |= 4;
    }

    if (auth_pw & 1) {
        /* We could authenticate via password */
        if (libssh2_userauth_password(pCtrl->pSshSession, pusername, ppasswd)) {
            dbgprint("Authentication by password failed!");
			return ERR_SSH_AUTH;
        }
    } else if (auth_pw & 2) {
        /* Or via keyboard-interactive */
        if (libssh2_userauth_keyboard_interactive(pCtrl->pSshSession, pusername, &kbd_callback) ) {
            dbgprint("Authentication by keyboard-interactive failed!");
			return ERR_SSH_AUTH;
        }
    } else if (auth_pw & 4) {
        /* Or by public key */
        if (libssh2_userauth_publickey_fromfile(pCtrl->pSshSession, pusername, "~/.ssh/id_rsa.pub", "~/.ssh/id_rsa", ppasswd)) {
            dbgprint("Authentication by public key failed!");
			return ERR_SSH_AUTH;
        }
    } else {
    	return ERR_SSH_AUTH;
    }

    /* Request a shell */
    if (!(pCtrl->pSshChannel = libssh2_channel_open_session(pCtrl->pSshSession))) {
        dbgprint("Unable to open a session");
		return ERR_SSH_SESSION;
    }

    /* Some environment variables may be set,
     * It's up to the server which ones it'll allow though
     */
    libssh2_channel_setenv(pCtrl->pSshChannel, "FOO", "bar");

    /* Request a terminal with 'vanilla' terminal emulation
     * See /etc/termcap for more options
     */
	if (libssh2_channel_request_pty(pCtrl->pSshChannel, "ANSI")) {
        dbgprint("Failed requesting pty");
		return ERR_SSH_PTY;
    }

    /* Open a SHELL on that pty */
    if (libssh2_channel_shell(pCtrl->pSshChannel)) {
        dbgprint("Unable to request shell on allocated pty");
		return ERR_SSH_CHANNEL;
    }

	/*回车，跳过登录界面的一些提示信息  */
    libssh2_channel_write(pCtrl->pSshChannel, "\r", strlen("\r"));
	Sleep(100);
    libssh2_channel_write(pCtrl->pSshChannel, "\r", strlen("\r"));
	
	Sleep(1000);
	
	char buffer[4096];
	libssh2_channel_read(pCtrl->pSshChannel, buffer, sizeof(buffer));
	

	libssh2_session_set_blocking(pCtrl->pSshSession, 0);

	//命令处理
	dbgprint("ssh create thread...ip=%s, port=%d", phostname, port);
	pCtrl->hSshThread = CreateThread(NULL, 0, ssh_sessionthread, pCtrl, 0, NULL);
	if (NULL == pCtrl->hSshThread)
	{
		dbgprint("ssh create thread fail...ip=%s, port=%d", phostname, port);
		return -1;
	}
	
	Sleep(1000);
	//资源在子线程退出时释放
	return 0;
}



int ssh_sessioninit(char *phostname, char *pusername, char *ppasswd, unsigned short port, int *pCtrlID)
{
	int ret = 0;
	TCHAR szName[128];

	int ctrlID;
	for (ctrlID = 0; ctrlID < MAX_SSH_CTRL_CNT; ctrlID++)
	{
		if (!g_astSshCtrl[ctrlID].IsUsed)
		{
			break;
		}
	}

	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		return -1;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];

	if (0 != ssh_runtimeInit())
	{
		return -1;
	}
	
	pCtrl->IsUsed = true;

	wsprintf(szName, _T("sshrequest_0x%x_%d"), GetCurrentThreadId(), ctrlID);
	pCtrl->hReqEvent = CreateEvent(NULL, FALSE, FALSE, szName);
	if (NULL == pCtrl->hReqEvent)
	{
		ssh_sessiondeinit(ctrlID);
		return -1;
	}

	wsprintf(szName, _T("sshreply_0x%x_%d"), GetCurrentThreadId(), ctrlID);
	pCtrl->hReplyEvent = CreateEvent(NULL, FALSE, FALSE, szName);
	if (NULL == pCtrl->hReplyEvent)
	{
		ssh_sessiondeinit(ctrlID);
		return -1;
	}

	wsprintf(szName, _T("mInput_0x%x_%d"), GetCurrentThreadId(), ctrlID);
	pCtrl->hInputMutex= CreateMutex(NULL, FALSE, szName);
	if (NULL == pCtrl->hInputMutex)
	{
		ssh_sessiondeinit(ctrlID);
		return -1;
	}

	wsprintf(szName, _T("mOutput_0x%x_%d"), GetCurrentThreadId(), ctrlID);
	pCtrl->hOutputMutex= CreateMutex(NULL, FALSE, szName);
	if (NULL == pCtrl->hOutputMutex)
	{
		ssh_sessiondeinit(ctrlID);
		return -1;
	}
	
	memset(pCtrl->szInput, 0, sizeof(pCtrl->szInput));
	memset(pCtrl->szOutput, 0, sizeof(pCtrl->szOutput));
	pCtrl->IsReady = false;
	
	pCtrl->hSshThread = NULL;
	pCtrl->socket = -1;
	pCtrl->pSshSession = NULL;
	pCtrl->pSshChannel = NULL;
	pCtrl->closing = false;	

	dbgprint("ssh_startsession...ip=%s, port=%d", phostname, port);
	ret = ssh_startsession(pCtrl, phostname, pusername, ppasswd, port);
	if (0 != ret)
	{
		ssh_sessiondeinit(ctrlID);
		ssh_runtimeDeInit();
		return ret;
	}

	Sleep(500);
	char szOutput[1024];
	ssh_executecmd(ctrlID, "", szOutput, sizeof(szOutput), 500);
	//printf(szOutput);

	pCtrl->IsReady = true;	
	
	*pCtrlID = ctrlID;
	return 0;
}

void ssh_sessiondeinit(int ctrlID)
{
	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		return;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];
	if (!pCtrl->IsUsed)
	{
		return;
	}
	
	if (pCtrl->hSshThread)
	{
		ssh_closesession(pCtrl);
	}
		
	if (pCtrl->hReqEvent)
		CloseHandle(pCtrl->hReqEvent);
	if (pCtrl->hReplyEvent)
		CloseHandle(pCtrl->hReplyEvent);
	if (pCtrl->hInputMutex)
		CloseHandle(pCtrl->hInputMutex);
	if (pCtrl->hOutputMutex)
		CloseHandle(pCtrl->hOutputMutex);
	if (pCtrl->hSshThread)
		CloseHandle(pCtrl->hSshThread);
	if (pCtrl->pSshChannel)
		libssh2_channel_free(pCtrl->pSshChannel);	
	if (pCtrl->pSshSession)
		libssh2_session_free(pCtrl->pSshSession);
	if (pCtrl->socket > 0)
		closesocket(pCtrl->socket);

	pCtrl->hReqEvent = NULL;
	pCtrl->hReplyEvent = NULL;
	pCtrl->hInputMutex = NULL;
	pCtrl->hOutputMutex = NULL;
	pCtrl->hSshThread = NULL;
	pCtrl->socket = -1;
	pCtrl->pSshSession = NULL;
	pCtrl->pSshChannel = NULL;

	pCtrl->IsReady = false;

	pCtrl->IsUsed = false;

	ssh_runtimeDeInit();
}

int ssh_executecmd(int ctrlID, char *pcmd, char *pretstr, int maxretlen, int timeout)
{
	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		dbgprint("invalid session id %d", ctrlID);
		return -1;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];
	if (!pCtrl->IsUsed)
	{
		dbgprint("invalid session id %d", ctrlID);
		return -1;
	}

	int cost = 0;
	while (!pCtrl->IsReady)
	{
		Sleep(1);
		cost++;

		if (cost > timeout)
		{
			break;
		}
	}
	
	if (!pCtrl->IsReady)
	{
		dbgprint("session id %d busy", ctrlID);
		return ERR_SSH_NOT_READY;
	}

	pCtrl->timeout = timeout;
		
	ssh_writeinput(pCtrl, pcmd);
	SetEvent(pCtrl->hReqEvent);
	//WaitForSingleObject(pCtrl->hReplyEvent, 15000);
	if (WAIT_OBJECT_0 == WaitForSingleObject(pCtrl->hReplyEvent, 15*1000))
	{
		ssh_readoutput(pCtrl, pretstr, maxretlen);
		return 0;
	}
	else
	{
		*pretstr = '\0';
		dbgprint("wait reply timeout");
		return -1;
	}
}

int ssh_getfile(int ctrlID, char *pSrcpath, char *pDstpath, int timeout)
{
	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		dbgprint("invalid session id %d", ctrlID);
		return -1;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];
	if (!pCtrl->IsUsed)
	{
		dbgprint("invalid session id %d", ctrlID);
		return -1;
	}

	int retry = 0;
	while (!pCtrl->IsReady)
	{
		Sleep(5000);
		retry++;

		if (retry > 3)
		{
			break;
		}
	}
	
	if (!pCtrl->IsReady)
	{
		dbgprint("session id %d busy", ctrlID);
		return ERR_SSH_NOT_READY;
	}

	//封装命令
	char szGetcmd[MAX_SSH_CMD_LEN] = {0};
	sprintf_s(szGetcmd, sizeof(szGetcmd)-1, "%s|%s|%s", SCP_GET_MAGIC_STRING, pSrcpath, pDstpath);
		
	ssh_writeinput(pCtrl, szGetcmd);
	SetEvent(pCtrl->hReqEvent);
	//WaitForSingleObject(pCtrl->hReplyEvent, 15000);
	if (WAIT_OBJECT_0 == WaitForSingleObject(pCtrl->hReplyEvent, timeout))
	{
		return 0;
	}
	else
	{
		dbgprint("wait reply timeout");
		return -1;
	}
}

int ssh_putfile(int ctrlID, char *pSrcpath, char *pDstpath, int timeout)
{
	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		dbgprint("invalid session id %d", ctrlID);
		return -1;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];
	if (!pCtrl->IsUsed)
	{
		dbgprint("invalid session id %d", ctrlID);
		return -1;
	}

	int retry = 0;
	while (!pCtrl->IsReady)
	{
		Sleep(5000);
		retry++;

		if (retry > 3)
		{
			break;
		}
	}
	
	if (!pCtrl->IsReady)
	{
		dbgprint("session id %d busy", ctrlID);
		return ERR_SSH_NOT_READY;
	}

	//封装命令
	char szPutcmd[MAX_SSH_CMD_LEN] = {0};
	sprintf_s(szPutcmd, sizeof(szPutcmd), "%s|%s|%s", SCP_PUT_MAGIC_STRING, pSrcpath, pDstpath);
		
	ssh_writeinput(pCtrl, szPutcmd);
	SetEvent(pCtrl->hReqEvent);
	//WaitForSingleObject(pCtrl->hReplyEvent, 15000);
	if (WAIT_OBJECT_0 == WaitForSingleObject(pCtrl->hReplyEvent, timeout))
	{
		return 0;
	}
	else
	{
		dbgprint("wait reply timeout");
		return -1;
	}
}

