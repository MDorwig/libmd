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

#ifdef TEST
void CAsyncSocket::BeginAccept(CAsyncSocket * skt)
{
  assert(m_inreq == NULL);
  m_inreq = new AcceptReq(m_fd,this,&CAsyncSocket::AcceptComplete,skt);
}

void CAsyncSocket::AcceptComplete(AcceptReq & aio)
{

}

void	CAsyncSocket::BeginConnect(struct sockaddr * sa,socklen_t salen)
{
  ConnectReq * cr = new ConnectReq(m_fd,this,&CAsyncSocket::ConnectComplete);
	int res = connect(GetHandle(),sa,salen);
	if (res == -1)
	{
		SetLastError(errno);
		if (errno == EINPROGRESS)
		  m_outreq = cr ;
		else
		{
		  ConnectComplete(*cr);
		  delete cr;
		}
	}
	else
	{
    ConnectComplete(*cr);
    delete cr;
	}
}

void	CAsyncSocket::BeginConnect(const char * host,int port)
{
	struct sockaddr_in sain ;
	sain.sin_addr.s_addr = inet_addr(host);
	sain.sin_port = htons(port);
	sain.sin_family = AF_INET;
	BeginConnect((struct sockaddr*)&sain,sizeof sain);
}

void CAsyncSocket::ConnectComplete(ConnectReq & aio)
{
  int status = aio.EndConnect();
  if (status == 0)
  {

  }
}

void CAsyncSocket::BeginRead(char * buf,size_t size)
{
  assert(m_inreq == NULL);
  m_inreq = new ReadWriteReq(m_fd,this,&CAsyncSocket::ReadComplete,buf,size);
}

void CAsyncSocket::ReadComplete(ReadWriteReq & aio)
{
  int status = aio.EndRead();
  if (status == 0)
  {

  }
}

void CAsyncSocket::BeginWrite(char * buf,size_t size)
{
  assert(m_outreq == NULL);
  m_outreq = new ReadWriteReq(m_fd,this,&CAsyncSocket::WriteComplete,buf,size);
}

void CAsyncSocket::WriteComplete(ReadWriteReq & aio)
{
  int n = aio.EndWrite();
  if (n > 0)
  {
  }
}
#endif

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
