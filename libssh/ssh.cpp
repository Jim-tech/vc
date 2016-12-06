// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <tchar.h>
#include "stdio.h"
#include "stdlib.h"

#include <WS2tcpip.h>
#include <winsock2.h>
#include "libssh2.h"
#include "ssh.h"

//#include ".\..\emtest\Hal\include\bructrl.h"


#define  MAX_SSH_CTRL_CNT  128

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
	LIBSSH2_SESSION *pSshSession;
	LIBSSH2_CHANNEL *pSshChannel;
}SSH_CTRL_S;

SSH_CTRL_S g_astSshCtrl[MAX_SSH_CTRL_CNT];

//定义特殊的命令，用于标记是scp的操作
#define SCP_GET_MAGIC_STRING  "[*!scp-get!*]"
#define SCP_PUT_MAGIC_STRING  "[*!scp-put!*]"

void ssh_readinput(SSH_CTRL_S *pCtrl, char *pcmdstr, int maxbufferlen)
{
	WaitForSingleObject(pCtrl->hInputMutex,INFINITE);
	pcmdstr[0] = '\0';
	strncpy_s(pcmdstr, maxbufferlen-1, pCtrl->szInput, strlen(pCtrl->szInput)); 
	memset(pCtrl->szInput, 0, sizeof(pCtrl->szInput));
	ReleaseMutex(pCtrl->hInputMutex);
}

void ssh_writeinput(SSH_CTRL_S *pCtrl, char *pcmdstr)
{
	WaitForSingleObject(pCtrl->hInputMutex,INFINITE);
	pCtrl->szInput[0] = '\0';
	strncpy_s(pCtrl->szInput, sizeof(pCtrl->szInput)-1, pcmdstr, strlen(pcmdstr)); 
	ReleaseMutex(pCtrl->hInputMutex);
}

void ssh_readoutput(SSH_CTRL_S *pCtrl, char *pcmdstr, int maxbufferlen)
{
	WaitForSingleObject(pCtrl->hOutputMutex,INFINITE);

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
	WaitForSingleObject(pCtrl->hOutputMutex,INFINITE);
	pCtrl->szOutput[0] = '\0';
	strncpy_s(pCtrl->szOutput, sizeof(pCtrl->szOutput)-1, pcmdstr, strlen(pcmdstr)); 
	ReleaseMutex(pCtrl->hOutputMutex);
}


void ssh_closesession(SSH_CTRL_S *pCtrl)
{
	TerminateThread(pCtrl->hSshThread, 0);
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

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

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

    rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

    return rc;
}

int ssh_session_getfile(LIBSSH2_SESSION *session, char *pInput)
{
	int rc;
	
	char *pSrc = strstr(pInput, ":");
	if (NULL == pSrc)
	{
        printf("\r\n[%d]para invalid", __LINE__);
		return -1;
	}
	else
	{
		*pSrc = '\0';
		pSrc += strlen(":");
	}

	char *pDst = strstr(pSrc, ":");
	if (NULL == pDst)
	{
		printf("\r\n[%d]para invalid", __LINE__);
        return -1;
	}
	else
	{
		*pDst = '\0';
		pDst += strlen(":");
	}

	LIBSSH2_CHANNEL *channel;
	struct stat fileinfo;
	off_t got=0;
	channel = libssh2_scp_recv(session, pSrc, &fileinfo);
    if (!channel) {
		printf("\r\n[%d]para invalid", __LINE__);
        return -1;
    }

	FILE *fp = NULL;
	rc = fopen_s(&fp, pDst, "wb+");
	if (NULL == fp || rc != 0)
	{
		printf("\r\n[%d]para invalid[%s]", __LINE__, pDst);
		libssh2_channel_free(channel);
        return -1;
	}

	while(got < fileinfo.st_size) 
	{
		char mem[1024];
		int amount=sizeof(mem);

		if((fileinfo.st_size -got) < amount) 
		{
			amount = fileinfo.st_size -got;
		}

		rc = libssh2_channel_read(channel, mem, amount);
		if(rc > 0) 
		{
			//写文件
			fwrite(mem, 1, rc, fp);
		}
		else if(rc < 0) 
		{
			break;
		}
		got += rc;
	}

    libssh2_channel_free(channel);
	fclose(fp);
	return 0;	
}

int ssh_session_putfile(LIBSSH2_SESSION *session, char *pInput)
{
	int rc;
	
	char *pSrc = strstr(pInput, ":");
	if (NULL == pSrc)
	{
        printf("\r\n[%d]para invalid", __LINE__);
		return -1;
	}
	else
	{
		*pSrc = '\0';
		pSrc += strlen(":");
	}

	char *pDst = strstr(pSrc, ":");
	if (NULL == pDst)
	{
		printf("\r\n[%d]para invalid", __LINE__);
        return -1;
	}
	else
	{
		*pDst = '\0';
		pDst += strlen(":");
	}

	LIBSSH2_CHANNEL *channel;
	struct stat fileinfo;
	stat(pSrc, &fileinfo);

    /* Send a file via scp. The mode parameter must only have permissions! */
    channel = libssh2_scp_send(session, pDst, fileinfo.st_mode & 0777,
                               (unsigned long)fileinfo.st_size);

    if (!channel) 
	{
		printf("\r\n[%d]para invalid", __LINE__);
        return -1;			
    }
	
	FILE *fp = NULL;
	rc = fopen_s(&fp, pSrc, "rb");
	if (NULL == fp || rc != 0)
	{
		printf("\r\n[%d]para invalid", __LINE__);
		libssh2_channel_free(channel);
		return -1;
	}

    do {
		char mem[1024];
        int nread = fread(mem, 1, sizeof(mem), fp);
        if (nread <= 0) {
            break;
        }

		printf("\r\n[%d] read[%d]", __LINE__, nread);
		
		char *ptr = mem;

        do {
            /* write the same data over and over, until error or completion */
            rc = libssh2_channel_write(channel, ptr, nread);
            if (rc < 0) {
                break;
            }
            else {
				
                /* rc indicates how many bytes were written this time */
                ptr += rc;
                nread -= rc;

				printf("\r\n[%d] write[%d]", __LINE__, ptr - mem);
            }
        } while (nread);
    } while (1);

    libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);
    libssh2_channel_free(channel);
	fclose(fp);
	
	return 0;
}

int ssh_session_execli(LIBSSH2_CHANNEL *channel, LIBSSH2_SESSION *session, char *pInput, char *pOutput)
{
	int rc;
	char buffer[0x4000];
	
	rc = libssh2_channel_write(channel, pInput, strlen(pInput) + 1);
    if (rc < 0) {
        printf("\r\n[%d]para invalid", __LINE__);
		return -1;
    }

	Sleep(100);
		
	int OutputCnt = 0;
	rc = libssh2_channel_read(channel, buffer, sizeof(buffer));
    if( rc > 0 )
    {
		if (MAX_SSH_CMD_LEN <= rc)
		{
			rc = MAX_SSH_CMD_LEN - 1;
		}

		//回显中前一部分是输入的命令
        for(int i=0; i < rc; ++i )
        {
			if ((unsigned int)i > strlen(pInput))
			{
				pOutput[OutputCnt++] = buffer[i];
			}
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

	//printf("\r\nenter ssh_session");
	while (1)
	{
		WaitForSingleObject(pCtrl->hReqEvent, INFINITE);

		memset(szCmdInput, 0, sizeof(szCmdInput));
		ssh_readinput(pCtrl, szCmdInput, sizeof(szCmdInput));

		if (szCmdInput == strstr(szCmdInput, SCP_GET_MAGIC_STRING))
		{
			rc = ssh_session_getfile(session, szCmdInput);
			if (0 != rc)
			{
				printf("\r\n[%d]para invalid", __LINE__);
			}
			
            SetEvent(pCtrl->hReplyEvent);
			continue;
		}
		else if (szCmdInput == strstr(szCmdInput, SCP_PUT_MAGIC_STRING))
		{
			rc = ssh_session_putfile(session, szCmdInput);
			if (0 != rc)
			{
				printf("\r\n[%d]para invalid", __LINE__);
			}
			
			SetEvent(pCtrl->hReplyEvent);
			continue;
		}
		else
		{
			memset(szCmdOutput, 0, sizeof(szCmdOutput));
			strcat_s(szCmdInput, sizeof(szCmdInput), "\n");
			printf("\r\n[%d] input=[%s]", __LINE__, szCmdInput);
			rc = ssh_session_execli(channel, session, szCmdInput, szCmdOutput);
			if (0 != rc)
			{
				printf("\r\n[%d]para invalid", __LINE__);
			}
			
			szCmdOutput[strlen(szCmdOutput)] = '\0';
			ssh_writeoutput(pCtrl, szCmdOutput);
			//printf("\r\nOutcmd=%s", szCmdOutput);
			
			SetEvent(pCtrl->hReplyEvent);	
			continue;
		}

		if (libssh2_channel_eof(channel))
		{
			break;
		}
	}

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
	WSADATA wsadata;
	int err;

	/*只在第一次使用的时候初始化，最后一次退出时去初始化*/
	for (int ctrlID = 0; ctrlID < MAX_SSH_CTRL_CNT; ctrlID++)
	{
		if (g_astSshCtrl[ctrlID].IsUsed)
		{
			return 0;
		}
	}
	
    err = WSAStartup(MAKEWORD(2,0), &wsadata);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return ERR_WSA_START;
    }
	
    rc = libssh2_init(0);
    if (rc != 0) {
        printf("libssh2 initialization failed (%d)\n", rc);
		WSACleanup( );
        return ERR_LIBSSH_INIT;
    }

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
	
	WSACleanup( );

	libssh2_exit();
}

int ssh_startsession(SSH_CTRL_S *pCtrl, char *phostname, char *pusername, char *ppasswd)
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
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if (connect(pCtrl->socket, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0) 
	{
        printf("failed to connect!\n");
        return ERR_CONN_FAIL;
    }

    /* Create a session instance and start it up. This will trade welcome
     * banners, exchange keys, and setup crypto, compression, and MAC layers
     */
    pCtrl->pSshSession = libssh2_session_init();
    if (libssh2_session_handshake(pCtrl->pSshSession, pCtrl->socket)) {
        printf("Failure establishing SSH session\n");
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
            printf("\tAuthentication by password failed!\n");
			return ERR_SSH_AUTH;
        }
    } else if (auth_pw & 2) {
        /* Or via keyboard-interactive */
        if (libssh2_userauth_keyboard_interactive(pCtrl->pSshSession, pusername, &kbd_callback) ) {
            printf("\tAuthentication by keyboard-interactive failed!\n");
			return ERR_SSH_AUTH;
        }
    } else if (auth_pw & 4) {
        /* Or by public key */
        if (libssh2_userauth_publickey_fromfile(pCtrl->pSshSession, pusername, "~/.ssh/id_rsa.pub", "~/.ssh/id_rsa", ppasswd)) {
            printf("\tAuthentication by public key failed!\n");
			return ERR_SSH_AUTH;
        }
    } else {
    	return ERR_SSH_AUTH;
    }

    /* Request a shell */
    if (!(pCtrl->pSshChannel = libssh2_channel_open_session(pCtrl->pSshSession))) {
        printf("Unable to open a session\n");
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
        printf("Failed requesting pty\n");
		return ERR_SSH_PTY;
    }

    /* Open a SHELL on that pty */
    if (libssh2_channel_shell(pCtrl->pSshChannel)) {
        printf("Unable to request shell on allocated pty\n");
		return ERR_SSH_CHANNEL;
    }

	/*回车，跳过登录界面的一些提示信息  */
    libssh2_channel_write(pCtrl->pSshChannel, "\r", strlen("\r"));
	Sleep(300);
	char buffer[4096];
	libssh2_channel_read(pCtrl->pSshChannel, buffer, sizeof(buffer));
	Sleep(300);
    libssh2_channel_write(pCtrl->pSshChannel, "\r", strlen("\r"));
	Sleep(300);
	libssh2_channel_read(pCtrl->pSshChannel, buffer, sizeof(buffer));
	
	//命令处理
	pCtrl->hSshThread = CreateThread(NULL, 0, ssh_sessionthread, pCtrl, 0, NULL);

	//资源在子线程退出时释放
	return 0;
}



int ssh_sessioninit(char *phostname, char *pusername, char *ppasswd, int *pCtrlID)
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

	wsprintf(szName, _T("sshrequest%d"), ctrlID);
	pCtrl->hReqEvent = CreateEvent(NULL, FALSE, FALSE, szName);
	if (NULL == pCtrl->hReqEvent)
	{
		ssh_sessiondeinit(ctrlID);
		return -1;
	}

	wsprintf(szName, _T("sshreply%d"), ctrlID);
	pCtrl->hReplyEvent = CreateEvent(NULL, FALSE, FALSE, szName);
	if (NULL == pCtrl->hReplyEvent)
	{
		ssh_sessiondeinit(ctrlID);
		return -1;
	}

	wsprintf(szName, _T("mInput%d"), ctrlID);
	pCtrl->hInputMutex= CreateMutex(NULL, FALSE, szName);
	if (NULL == pCtrl->hInputMutex)
	{
		ssh_sessiondeinit(ctrlID);
		return -1;
	}

	wsprintf(szName, _T("mOutput%d"), ctrlID);
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
	ret = ssh_startsession(pCtrl, phostname, pusername, ppasswd);
	if (0 != ret)
	{
		ssh_sessiondeinit(ctrlID);
		ssh_runtimeDeInit();
		return ret;
	}

	Sleep(500);
	char szOutput[1024];
	ssh_executecmd(ctrlID, "", szOutput, sizeof(szOutput));
	printf(szOutput);

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

int ssh_executecmd(int ctrlID, char *pcmd, char *pretstr, int maxretlen)
{
	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		return -1;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];
	if (!pCtrl->IsUsed)
	{
		return -1;
	}
	
	if (!pCtrl->IsReady)
		return ERR_SSH_NOT_READY;
		
	ssh_writeinput(pCtrl, pcmd);
	SetEvent(pCtrl->hReqEvent);
	//WaitForSingleObject(pCtrl->hReplyEvent, INFINITE);
	if (WAIT_OBJECT_0 == WaitForSingleObject(pCtrl->hReplyEvent, 10*1000))
	{
		ssh_readoutput(pCtrl, pretstr, maxretlen);
		return 0;
	}
	else
	{
		*pretstr = '\0';
		return -1;
	}
}

int ssh_getfile(int ctrlID, char *pSrcpath, char *pDstpath)
{
	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		return -1;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];
	if (!pCtrl->IsUsed)
	{
		return -1;
	}
	
	if (!pCtrl->IsReady)
		return ERR_SSH_NOT_READY;

	//封装命令
	char szGetcmd[MAX_SSH_CMD_LEN] = {0};
	sprintf_s(szGetcmd, sizeof(szGetcmd)-1, "%s:%s:%s", SCP_GET_MAGIC_STRING, pSrcpath, pDstpath);
		
	ssh_writeinput(pCtrl, szGetcmd);
	SetEvent(pCtrl->hReqEvent);
	//WaitForSingleObject(pCtrl->hReplyEvent, INFINITE);
	if (WAIT_OBJECT_0 == WaitForSingleObject(pCtrl->hReplyEvent, 30*1000))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int ssh_putfile(int ctrlID, char *pSrcpath, char *pDstpath)
{
	if (MAX_SSH_CTRL_CNT <= ctrlID)
	{
		return -1;
	}

	SSH_CTRL_S *pCtrl = &g_astSshCtrl[ctrlID];
	if (!pCtrl->IsUsed)
	{
		return -1;
	}
	
	if (!pCtrl->IsReady)
		return ERR_SSH_NOT_READY;

	//封装命令
	char szPutcmd[MAX_SSH_CMD_LEN] = {0};
	sprintf_s(szPutcmd, sizeof(szPutcmd), "%s:%s:%s", SCP_PUT_MAGIC_STRING, pSrcpath, pDstpath);
		
	ssh_writeinput(pCtrl, szPutcmd);
	SetEvent(pCtrl->hReqEvent);
	//WaitForSingleObject(pCtrl->hReplyEvent, INFINITE);
	if (WAIT_OBJECT_0 == WaitForSingleObject(pCtrl->hReplyEvent, 30*1000))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

