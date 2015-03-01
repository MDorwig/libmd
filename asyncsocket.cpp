#include "asyncsocket.h"
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

CAsyncSocket::CAsyncSocket()
{
	m_eventmask=0;
	m_type		=	0;
	m_port 		= 0;
	m_state		= SKT_IDLE;
}

void CAsyncSocket::OnConnect(int nerr)
{
}

void CAsyncSocket::OnAccept(int nerr)	
{
}

void CAsyncSocket::OnClose(int nerr)
{
	Close(); 
}

void CAsyncSocket::OnReceive(int nerr)
{
}

void CAsyncSocket::OnSend(int nErrorCode)
{
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

int CAsyncSocket::Create(int port,int type,int events)
{
	int res ;
	struct sockaddr_in sain ;
	m_eventmask=events;
	m_type		=	type;
	m_port    = port;
	m_state		= SKT_IDLE;
	res       = socket(AF_INET,type,0);
	if (res!= -1)
	{
		Attach(res,0) ;
		SetBlocking(0);
		if (port != 0)
		{
			sain.sin_addr.s_addr = htonl(INADDR_ANY) ;
			sain.sin_family = AF_INET;
			sain.sin_port   = htons(port);
			m_state = SKT_BOUND;
			SetOption(SO_REUSEADDR,1);
			res = Bind((struct sockaddr*)&sain,sizeof sain);
			if (res != -1)
			{
				if (m_type == SOCK_DGRAM)
					AsyncSelect(events);
			}
			else
			{
				SetLastError(errno);
				m_state = SKT_IDLE;
			}
		}
	}
	return res ;
}

void CAsyncSocket::AsyncSelect(int events)
{
	int pm = 0 ;
	m_eventmask = events;
	if (events & (FD_READ|FD_ACCEPT))
		pm |= POLLIN;
	if (events & (FD_WRITE|FD_CONNECT))
		pm |= POLLOUT;
	SetPollMask(pm);
	//ClrPollMask(~pm);
}

CAsyncSocket::~CAsyncSocket()
{
	
}

int CAsyncSocket::Listen(int backlog)
{
	int res ;
	res = listen(GetHandle(),backlog);
	if (res != -1)
	{
		m_state = SKT_LISTEN;
		AsyncSelect(m_eventmask);
	}
	else
		SetLastError(errno);
	return res ;
}

int	CAsyncSocket::Bind(const struct sockaddr * sa,socklen_t salen)
{
	int res ;
	res = bind(GetHandle(),sa,salen);
	if (res != -1)
		m_state = SKT_BOUND;
	else
		SetLastError(errno);
	return res ;
}

int	CAsyncSocket::Accept(CAsyncSocket & accskt,struct sockaddr * sa,socklen_t * salen)
{
	int res ;
	res = accept(GetHandle(),sa,salen);
	if (res != -1)
	{
		accskt.m_type  		= m_type;
		accskt.m_port     = m_port;
		accskt.m_eventmask= m_eventmask & ~FD_CONNECT;
		accskt.m_state 		= SKT_CONNECTED;
		accskt.Attach(res,0) ;
		accskt.AsyncSelect(accskt.m_eventmask);
	}
	else
	{
		SetLastError(errno);
	}
	SetPollMask(POLLIN);
	return res ;
}

int	CAsyncSocket::Connect(struct sockaddr * sa,socklen_t salen)
{
	int res ;
	res = connect(GetHandle(),sa,salen);
	if (res == -1)
	{
		SetLastError(errno);
		if (errno == EAGAIN || errno == EINPROGRESS)
		{
			m_state = SKT_CONNECTING;
			res = 0 ;
			AsyncSelect(m_eventmask);
		}
	}
	else
	{
		m_state	= SKT_CONNECTED;
		AsyncSelect(m_eventmask);
		OnEvent(FD_CONNECT,0);
	}
	return res ;
}

int	CAsyncSocket::Connect(const char * host,int port)
{
	struct sockaddr_in sain ;
	sain.sin_addr.s_addr = inet_addr(host);
	sain.sin_port = htons(port);
	sain.sin_family = AF_INET;
	return Connect((struct sockaddr*)&sain,sizeof sain);
}

int CAsyncSocket::Receive(void * buf,size_t size,int flags)
{
	int res = recv(GetHandle(),buf,size,flags);
	if (res == -1)
		SetLastError(errno);
	else if (res == 0)
		OnEvent(FD_CLOSE,0);
	return res ;
}

int CAsyncSocket::Send(const void * buf,size_t size,int flags)
{
	int res = send(GetHandle(),buf,size,flags);
	if (res == -1)
		SetLastError(errno);
	else
		SetPollMask(GetPollMask()|POLLOUT);
	return res ;
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

void CAsyncSocket::Dispatch(int events,int nerr)
{
	if (events & FD_CONNECT)
		OnConnect(nerr);
	if (events & FD_ACCEPT)
		OnAccept(nerr);
	if (events & FD_READ)
		OnReceive(nerr);
	if (events & FD_WRITE)
		OnSend(nerr);
	if (events & FD_CLOSE)
		OnClose(nerr);
}

#define CASETXT(x)	case x : return #x 

static const char * StateName(sktstates st)
{
	switch(st)
	{
		CASETXT(SKT_IDLE);
		CASETXT(SKT_LISTEN);
		CASETXT(SKT_BOUND);
		CASETXT(SKT_CONNECTING);
		CASETXT(SKT_CONNECTED);
		CASETXT(SKT_CLOSED);
	}
	return "SKT_UNKONW";
}

void CAsyncSocket::OnPollErr()
{
	printf("POLLERR in state %s\n",StateName(m_state));
	OnEvent(FD_CLOSE,0);
}

void CAsyncSocket::OnPollHup()
{
	int nerr = 0;
	printf("POLLHUP in state %s\n",StateName(m_state));
	switch(m_state)
	{
		case SKT_CONNECTED:
			OnEvent(FD_CLOSE,0);
		break ;
		
		case	SKT_CONNECTING:
		{
			GetOption(SO_ERROR,nerr);
			OnEvent(FD_CONNECT,nerr);
		}
		break;
		
		default:
		break ;
	}
}

void CAsyncSocket::OnEvent(int events,int nerr)
{
	Dispatch(events,nerr);
}

void CAsyncSocket::OnPollIn()
{
	switch(m_state)
	{
		case	SKT_LISTEN:
			OnEvent(FD_ACCEPT,0);
		break ;
		
		case	SKT_BOUND:
			if (m_type == SOCK_DGRAM)
				OnEvent(FD_READ,0);
		break ;
		
		case	SKT_CONNECTING:
			OnEvent(FD_CONNECT,0);
		break ;
		
		case	SKT_CONNECTED:
			m_state = SKT_CONNECTED;
			OnEvent(FD_READ,0);
		break ;
		
		default:
			printf("POLLIN in state %s\n",StateName(m_state));
		break ;
	}
}

void CAsyncSocket::OnPollOut()
{
	switch(m_state)
	{
		case	SKT_BOUND:
			if (m_type == SOCK_DGRAM)
				OnEvent(FD_WRITE,0);
		break ;
		
		case	SKT_CONNECTING:
			m_state = SKT_CONNECTED;
			OnEvent(FD_CONNECT,0);
		break ;
		
		case	SKT_CONNECTED:
			OnEvent(FD_WRITE,0);
		break;
		
		default:
			printf("POLLOUT in state %s\n",StateName(m_state));
		break ;
	}
}
