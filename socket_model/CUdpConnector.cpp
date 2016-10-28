/*************************************************************************
	> File Name: CUdpConnector.cpp
	> Author: tony
	> Mail: 445241843@qq.com 
	> Created Time: 2016年10月28日 星期五 14时54分05秒
 ************************************************************************/

#include "CUdpConnector.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

namespace lce
{
	const char* CUdpConnector::getLocalIp()
	{
		static char ip[20] = {0};
		int i;

		int s = socket(AF_INET, SOCK_DGRAM, 0);
		for(i = 1; ; ++i)
		{
			struct ifreq ifr;
			struct sockaddr_in *sin = (struct sockaddr in *)&ifr.ifr_addr;
			ifr.ifr_ifindex = i;
			if(ioctl(s, SIOCGIFNAME, &ifr) < 0)
				break;
			if(ioctl(s, SIOCGIFADDR, &ifr) < 0)
				continue;
			strcpy(ip, inet_ntoa(sin->sin_addr));
			if(0 == strncmp(ip, "10.", 3) || 0 == strncmp(ip, "172", 3) || 0 == strncmp(ip, "192", 3))
			{
				::close(s);
				return ip;
			}
			else
				continue;
		}
		::close(s);
		return NULL;
	}
	
	bool CUdpConnector::connect(const std::string &sSvrIp, const unsigned short wSvrPort, const std::string &sLocalIp, const unsigned short wLocalPort)
	{
		m_sSvrIp = sSvrIp;
		m_sSvrPort = sSvrPort;

		if((m_iFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Connect: socket error(%d):%s", errno, strerr(errno));
			return false;
		}

		int reuse = -1;
		setsockopt(m_iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int));

		struct sockaddr_in sockClientAddr;
		memset(&sockClientAddr, 0, sizeof(struct sockaddr_in));
		sockClientAddr.sin_family = AF_INET;
		if(sLocalIp.empty())
			sockClientAddr.sin_addr.s_addr = inet_addr(getLocalIp());
		else
			sockClientAddr.sin_addr.s_addr = inet_addr(sLocalIp.cstr());
		sockClientAddr.sin_port = htons(wLocalPort);
		if(bind(m_iFd, (struct sock_addr *)&sockClientAddr, sizeof(sockClientAddr)) < 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Connect: bind error(%d): %s", errno, strerr(errno));
			this->close();
			return false;
		}
		m_bConnect = false;
		return true;
	}

	int CUdpConnector::write(char *pData, const int iSize)
	{
		if(!m_bConnect)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "no create connect");
			return -1;
		}
		struct sockaddr_in sockSvrAddr;
		memset(&sockSvrAddr, 0, sizeof(sockSvrAddr));
		sockSvrAddr.sin_family = AF_INET;
		sockSvrAddr.sin_addr.s_addr = inet_addr(m_sSvrIp.cstr());
		sockSvrAddr.sin_port = htons(m_wSvrPort);

		int iSendSize = 0;
		iSendSize = ::sendto(m_iFd, pData, iSize, 0, (struct sockaddr *)&sockSvrAddr, sizeof(sockSvrAddr));
		if(iSendSize < 0)
		{
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "sendto error:%s", strerr(errno));
		//	this->close();
			return -1;
		}
		return iSendSize;

	}
	int CUdpConnector::read(char * pBuf, const int iBufSize, const time_t dwTimeout)
	{
		int iRe = 0;
		struct timeval tv;
		tv.tv_sec = dwTimeout / 1000;
		tv.tv_usec = (dwTimeout % 1000) * 1000;
		setsockopt(m_iFd, SOL_SOCKET, SO_REVTIMEO, &tv, sizeof(tv));

		int n = ::recvfrom(m_iFd, pBuf, iBufSize, 0, (struct sockaddr*)0, (socklen_t*)0);
		if(n > 0)
		{
			iRe = n;
		}
		else if(n < 0)
		{
			if(EWOULDBLOCK == n)		//!!!!!!
			{
				snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Read recvfrom over time");
				iRe = -1;
			}
			else
			{
				snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Read recvfrom error(%d): %s", errno, strerr(errno));
				iRe = -1;
			}
		}
		else
		{	
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Read recvfrom over time");
			iRe = -1;
		}
		return iRe;
	}

	int CUdpConnector::read(std::string &sBuf, const time_t dwTimeout)
	{
		int iRe = 0;
		int iBufSize = 65535;
		sBuf.resize(iBufSize);

		struct timeval tv;
		tv.tv_sec = dwTimeout / 1000;
		tv.tv_usec = (dwTimeout % 1000) * 1000;
		setsockopt(m_iFd, SOL_SOCKET, SO_REVTIMEO, &tv, sizeof(tv));

		int n = ::recvfrom(m_iFd, (char *)sBuf.data(), iBufSize, 0, (struct sockaddr*)0, (socklen_t*)0);
		if(n > 0)
		{
			s.Buf.resize(n);
			iRe = n;
		}
		else if(n < 0)
		{
			sBuf.clear();
			if(EWOULDBLOCK == n)		//!!!!!!
			{
				snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Read recvfrom over time");
				iRe = -1;
			}
			else
			{
				snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Read recvfrom error(%d): %s", errno, strerr(errno));
				iRe = -1;
			}
		}
		else
		{	
			sBuf.clear();
			snprintf(m_szErrMsg, sizeof(m_szErrMsg), "Read recvfrom over time");
			iRe = -1;
		}
		return iRe;
	}

	void CUdpConnector::close()
	{
		::close(m_iFd);
		m_iFd = -1;
		m_bConnect = false;

	}

};

