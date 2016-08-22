#include "asyncsocket.h"
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

CAsyncSocket::CAsyncSocket(CMsgQueue & q) : CFileHandle(q)
{
	m_type		=	0;
	m_port 		= 0;
}

int CAsyncSocket::SetOption(int name,int value)
{
	return setsockopt(GetHandle(), SOL_SOCKET, name,(char *)&value,sizeof value);
}

int CAsyncSocket::GetOption(int name,int & value)
{
	int res ;
	socklen_t len = sizeof value ;
	res = getsockopt(GetHandle(), SOL_SOCKET, name,(char *)&value,&len);
	if (res == -1)
		SetLastError(errno);
	return res ;
}

int CAsyncSocket::GetName(struct sockaddr * sa,socklen_t * salen)
{
	return getsockname(GetHandle(),sa,salen);
}

int CAsyncSocket::Create(int port,int type)
{
	int res ;
	struct sockaddr_in sain ;
	m_type		=	type;
	m_port    = port;
	res       = socket(AF_INET,type,0);
	if (res!= -1)
	{
		Attach(res) ;
		if (port != 0)
		{
			sain.sin_addr.s_addr = htonl(INADDR_ANY) ;
			sain.sin_family = AF_INET;
			sain.sin_port   = htons(port);
			SetOption(SO_REUSEADDR,1);
			res = Bind((struct sockaddr*)&sain,sizeof sain);
			if (res == -1)
				SetLastError(errno);
		}
	}
	return res ;
}

CAsyncSocket::~CAsyncSocket()
{

}

int CAsyncSocket::Listen(int backlog)
{
	int res ;
	res = listen(GetHandle(),backlog);
	if (res == -1)
		SetLastError(errno);
	return res ;
}

int	CAsyncSocket::Bind(const struct sockaddr * sa,socklen_t salen)
{
	int res ;
	res = bind(GetHandle(),sa,salen);
	if (res == -1)
		SetLastError(errno);
	return res ;
}

int	CAsyncSocket::Bind(int port)
{
	struct sockaddr_in sain;
	socklen_t salen	;
	sain.sin_addr.s_addr = INADDR_ANY;
	sain.sin_family = AF_INET;
	sain.sin_port   = htons(port);
	salen = sizeof sain;
	return Bind((struct sockaddr*)&sain,salen);
}

int CAsyncSocket::ShutDown(int how)
{
	int res ;
	res = shutdown(GetHandle(),how);
	if (res == -1)
		SetLastError(errno);
	return res ;
}

int CAsyncSocket::Close()
{
	return CFileHandle::Close();
}
