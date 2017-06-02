#include "stdafx.h"
#include "windows.h"
#include "ssh.h"

extern FILE           *fplog;

int bru_ssh_login(char *ipstr, unsigned short port, int *psession)
{
	char  sz_req[1024] = {0};
	char  sz_resp[1024] = {0};

	int  session = -1;
	int  ret = 0;

	char *user = NULL;
	char *pw = NULL;

	if (22 == port)
	{
		user = "root";
		pw = "root123";
	}
	else
	{
		user = "admin";
		pw = "admin";
	}
	
	ret = ssh_sessioninit(ipstr, user, pw, port, &session);
	if (0 != ret)
	{
		dbgprint("login to %s fail, ret=%d", ipstr, ret);
	}
	else
	{
		if (0 != strcmp("root", user))
		{
			ssh_executecmd(session, "admin", sz_resp, sizeof(sz_resp), 500);
			ssh_executecmd(session, "qpa;10@(", sz_resp, sizeof(sz_resp), 500);
			ssh_executecmd(session, "start-shell", sz_resp, sizeof(sz_resp), 500);
			ssh_executecmd(session, "sudo su -", sz_resp, sizeof(sz_resp), 500);

			ssh_executecmd(session, "cp -rf /etc/ssh/sshd_config /tmp/sshd_config", sz_resp, sizeof(sz_resp), 1000);
			ssh_executecmd(session, "sed -i \"s/PermitRootLogin no/#PermitRootLogin no/g\" /tmp/sshd_config", sz_resp, sizeof(sz_resp), 1000);
			ssh_executecmd(session, "sed -i \"s/Port 27149/Port 22/g\" /tmp/sshd_config", sz_resp, sizeof(sz_resp), 1000);
			ssh_executecmd(session, "/usr/sbin/sshd -f /tmp/sshd_config 2>&1 >/dev/null", sz_resp, sizeof(sz_resp), 1000);

			ssh_sessiondeinit(session);
			Sleep(2000);

			ret = ssh_sessioninit(ipstr, "root", "root123", 22, &session);
			if (0 != ret)
			{
				dbgprint("login to %s fail, ret=%d", ipstr, ret);
			}
		}
	}

	if (0 != ret)
	{
		return ret;
	}

	Sleep(1000);

	//ssh_executecmd(session, "mkdir -p /tmp/__backfs", sz_resp, sizeof(sz_resp), 500);
	//ssh_executecmd(session, "cp -rf /bin/busybox /tmp/", sz_resp, sizeof(sz_resp), 1000);
	//ssh_executecmd(session, "cp -rf /usr/bin/pgrep /tmp/", sz_resp, sizeof(sz_resp), 1000);
	
	#if  0	
	/*拷贝一些命令  */
	ssh_executecmd(session, "cp -rf /usr/sbin/ru_cmd /tmp/", sz_resp, sizeof(sz_resp), 1000);
	ssh_executecmd(session, "cp -rf /sbin/eeprom-tools /tmp/", sz_resp, sizeof(sz_resp), 1000);
	ssh_executecmd(session, "cp -rf /sbin/ImageUpgrade /tmp/", sz_resp, sizeof(sz_resp), 1000);
	ssh_executecmd(session, "cp -rf /usr/bin/pgrep /tmp/", sz_resp, sizeof(sz_resp), 1000);
	ssh_executecmd(session, "cp -rf /bin/busybox /tmp/", sz_resp, sizeof(sz_resp), 1000);
	ssh_executecmd(session, "cp -rf /sbin/version /tmp/", sz_resp, sizeof(sz_resp), 1000);
	#endif /* #if 0 */

	*psession = session;
	return 0;
}

void bru_ssh_logout(int session)
{
	ssh_sessiondeinit(session);
}

int bru_ssh_uploadfile(int session, char *psrc, char *pdst)
{
	return ssh_putfile(session, psrc, pdst, 120*1000);
}

void bru_ssh_led_slowflash(int session)
{
	char  sz_resp[1024] = {0};
	
	ssh_executecmd(session, "chmod 777 /tmp/ledctrl", sz_resp, sizeof(sz_resp), 500);
	Sleep(500);
	ssh_executecmd(session, "/tmp/ledctrl 1000000 2>&1 >/dev/null &", sz_resp, sizeof(sz_resp), 500);
}

void bru_ssh_ledfastflash(int session)
{
	char  sz_resp[1024] = {0};
	
	ssh_executecmd(session, "chmod 777 /tmp/ledctrl", sz_resp, sizeof(sz_resp), 500);
	Sleep(500);
	ssh_executecmd(session, "/tmp/ledctrl 300000 2>&1 >/dev/null &", sz_resp, sizeof(sz_resp), 500);
}

int bru_ssh_upgrade(int session)
{
	int   ret = 0;
	char  sz_req[1024] = {0};
	char  sz_resp[1024] = {0};

	ssh_executecmd(session, "pgrep phytest | xargs kill -9", sz_resp, sizeof(sz_resp), 500);
	ssh_executecmd(session, "pgrep lte_testmac | xargs kill -9", sz_resp, sizeof(sz_resp), 500);
	
	sprintf_s(sz_req, "ImageUpgrade /tmp/firmware.img raw > /tmp/updatelog 2>&1 & \r\n");
	ret = ssh_executecmd(session, sz_req, sz_resp, sizeof(sz_resp), 500);
	if (0 != ret)
	{
		dbgprint("upgrade fail, ret=%d", ret);
		return ret;
	}

	Sleep(5000);

	int seconds = 0;
	int cnt = 0;
	while (1)
	{
		Sleep(1000);
		seconds += 1;
		if (seconds > 300)
		{
			ret = -1;
			dbgprint("upgrade fail ret=%d", ret);
			break;
		}

		ret = ssh_executecmd(session, "pgrep ImageUpgrade", sz_resp, sizeof(sz_resp), 2000);
		if (0 != ret)
		{
			dbgprint("pgrep fail ret=%d", ret);
			continue;
		}

		dbgprint("resp=[%s]", sz_resp);

		int  pid = -1;
		char szpid[256] = {0};
		sscanf_s(sz_resp, "%s", szpid, sizeof(szpid));
		if ((1 == sscanf_s(szpid, "%d", &pid)) && (pid > 0))
		{
			dbgprint("upgrade is running");
			cnt = 0;
			continue;
		}

		cnt++;
		if (cnt < 5)
		{
			dbgprint("upgrade complete, check time = %d", cnt);
			Sleep(1000);
			continue;
		}

		break;
	}

	ssh_executecmd(session, "sync", sz_resp, sizeof(sz_resp), 500);
	Sleep(2000);

	ret = 0;
	cnt = 0;
	memset(sz_resp, 0, sizeof(sz_resp));
	while ((strlen(sz_resp) == 0) && cnt < 10)
	{
		ssh_executecmd(session, "echo xx >>/tmp/updatelog", sz_resp, sizeof(sz_resp), 500);
		
		cnt++;
		sprintf_s(sz_req, "tail -n 30 /tmp/updatelog | grep '###Upgrade'");
		ret = ssh_executecmd(session, sz_req, sz_resp, sizeof(sz_resp), 2000);
		if (0 != ret)
		{
			dbgprint("get upgrade log fail ret=%d", ret);
			continue;
		}

		dbgprint("resp=[%s]", sz_resp);
		
		if (NULL != strstr(sz_resp, "#Upgrade Success!#"))
		{
			break;
		}

		if (NULL != strstr(sz_resp, "#Upgrade Failed!#"))
		{
			break;
		}
		
	}

	if (cnt > 10)
	{
		dbgprint("cnt=%d, upgrade timeout", cnt);
		return -1;
	}
	
	if (NULL != strstr(sz_resp, "#Upgrade Success!#"))
	{
		ret = 0;
		dbgprint("upgrade ok ret=%d", ret);
		
	}
	else if (NULL != strstr(sz_resp, "#Upgrade Failed!#"))
	{
		ret = -1;
		dbgprint("upgrade fail ret=%d", ret);
	}	
	else
	{
		ret = 0;
		dbgprint("to be check");
	}
	
	return ret;
}

int bru_ssh_get_sn(int session, char *sn, int maxlen)
{
	int   ret = 0;
	char  sz_resp[1024] = {0};

	ret = ssh_executecmd(session, "eeprom-tools -t sn -r | sed 's/^.*[\\(]\\(.*\\)[\\)].*$/\\1/g'", sz_resp, sizeof(sz_resp), 2000);
	if (0 != ret)
	{
		dbgprint("get sn fail ret=%d", ret);
		return ret;
	}

	sscanf_s(sz_resp, "%s", sn, maxlen);
	dbgprint("sn=[%s]", sn);
	return 0;	
}

int bru_ssh_get_mac(int session, char *mac, int maxlen)
{
	int   ret = 0;
	char  sz_resp[1024] = {0};

	ret = ssh_executecmd(session, "eeprom-tools -t mac -r | sed 's/^.*[\\(]\\(.*\\)[\\)].*$/\\1/g'", sz_resp, sizeof(sz_resp), 2000);
	if (0 != ret)
	{
		dbgprint("get mac fail ret=%d", ret);
		return ret;
	}

	sscanf_s(sz_resp, "%s", mac, maxlen);
	dbgprint("mac=[%s]", mac);
	return 0;	
}

int bru_ssh_get_bootm(int session, int *p_bootm)
{
	int   ret = 0;
	char  sz_resp[1024] = {0};

	ret = ssh_executecmd(session, "eeprom-tools -t bootm -r|sed 's/^.*[\\(]\\(.*\\)[\\)].*$/\\1/g'", sz_resp, sizeof(sz_resp), 2000);
	if (0 != ret)
	{
		dbgprint("get bootm fail ret=%d", ret);
		return ret;
	}

	char szbootm[32] = {0};
	sscanf_s(sz_resp, "%s", szbootm, sizeof(szbootm));
	dbgprint("bootm=[%s]", szbootm);
	*p_bootm = strtoul(szbootm, 0, 0);
	dbgprint("*p_bootm=[0x%08X]", *p_bootm);
	return 0;	
}

int bru_ssh_set_macaddr(int session, char *pmacstr)
{
	int   ret = 0;
	char  sz_cmd[256] = {0};
	char  sz_resp[256] = {0};
	
	sprintf_s(sz_cmd, sizeof(sz_cmd)-1, "eeprom-tools -t mac -w %s", pmacstr);
	ret = ssh_executecmd(session, sz_cmd, sz_resp, sizeof(sz_resp), 500);
	if (0 != ret)
	{
		dbgprint("set macaddr fail ret=%d", ret);
		return ret;
	}

	return 0;		
}

int bru_ssh_get_curr_version(int session, int bootm, char *version, int maxlen)
{
	int   ret = 0;
	char  sz_resp[1024] = {0};

	ret = ssh_executecmd(session, "cat /etc/banner | grep 'V...R...C..B...' | awk '{print $NF}'", sz_resp, sizeof(sz_resp), 2000);
	if (0 != ret)
	{
		dbgprint("get version fail ret=%d", ret);
		return ret;
	}

	sscanf_s(sz_resp, "%s", version, maxlen);
	dbgprint("version=[%s]", version);
	return 0;		
}

int bru_ssh_checkback(int session, int bootm, char *version)
{
	int   ret = 0;
	char  sz_req[1024] = {0};
	char  sz_resp[1024] = {0};

	char  mtdname[32] = {0};
	if (0 == (bootm & 1)) //当前mtd6, 备区mtd8
	{
		sprintf_s(mtdname, sizeof(mtdname), "/dev/mtdblock8");
	}
	else
	{
		sprintf_s(mtdname, sizeof(mtdname), "/dev/mtdblock6");
	}

	ssh_executecmd(session, "mkdir -p /tmp/__backfs", sz_resp, sizeof(sz_resp), 500);
	
	sprintf_s(sz_req, "busybox mount -t jffs2 %s /tmp/__backfs &", mtdname);
	ret = ssh_executecmd(session, sz_req, sz_resp, sizeof(sz_resp), 500);
	if (0 != ret)
	{
		dbgprint("check %s fail", mtdname);
		return ret;
	}	

	int seconds = 0;
	int cnt = 0;
	while (1)
	{
		Sleep(1000);
		seconds += 1;
		if (seconds > 60)
		{
			ret = -1;
			dbgprint("mount %s fail ret=%d", mtdname, ret);
			break;
		}

		ret = ssh_executecmd(session, "pgrep busybox", sz_resp, sizeof(sz_resp), 2000);
		if (0 != ret)
		{
			dbgprint("pgrep busybox fail mtdname=%s ret=%d", mtdname, ret);
			continue;
		}

		int  pid = -1;
		char szpid[32] = {0};
		sscanf_s(sz_resp, "%s", szpid, sizeof(szpid));
		if ((1 == sscanf_s(szpid, "%d", &pid)) && (pid > 0))
		{
			dbgprint("mount %s is running", mtdname);
			cnt = 0;
			continue;
		}

		cnt++;
		if (cnt < 5)
		{
			dbgprint("mount complete, check time = %d", cnt);
			Sleep(1000);
			continue;
		}

		ret = 0;
		break;
	}

	if (0 != ret)
	{
		return -1;
	}

	cnt = 0;
	memset(sz_resp, 0, sizeof(sz_resp));

	while ((cnt < 10) && (strlen(sz_resp) == 0))
	{
		Sleep(1000);
		cnt++;
		
		sprintf_s(sz_req, "cat /tmp/__backfs/etc/banner | grep 'V...R...C..B...' | awk '{print $NF}'");
		ret = ssh_executecmd(session, sz_req, sz_resp, sizeof(sz_resp), 2000);
		if (0 != ret)
		{
			dbgprint("get %s version fail ret=%d", mtdname, ret);
			continue;
		}
	}

	dbgprint("%s version is %s", mtdname, sz_resp);
	if (NULL == strstr(sz_resp, version))
	{
		dbgprint("%s version not match the expect %s", mtdname, version);
		return -1;
	}

	ssh_executecmd(session, "chmod 777 /tmp/ledctrl", sz_resp, sizeof(sz_resp), 500);
	Sleep(500);
	ssh_executecmd(session, "/tmp/ledctrl 1000000 2>&1 >/dev/null &", sz_resp, sizeof(sz_resp), 500);
	return 0;	
}

void bru_ssh_complete(int session)
{
	char  sz_resp[1024] = {0};
	
	ssh_executecmd(session, "chmod 777 /tmp/ledctrl", sz_resp, sizeof(sz_resp), 500);
	Sleep(500);
	ssh_executecmd(session, "/tmp/ledctrl 1000000 2>&1 >/dev/null &", sz_resp, sizeof(sz_resp), 500);
	return;	
}

void bru_ssh_reboot(int session)
{
	char  sz_resp[1024] = {0};

	Sleep(1000);
	ssh_executecmd(session, "reboot", sz_resp, sizeof(sz_resp), 500);
	Sleep(1000);

	return;	
}
