#ifndef ASYNCSOCKET_H_
#define ASYNCSOCKET_H_

#include "filehandle.h"
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

class CAsyncSocket;
template<class T,class SKT> class AioAccept : public AioBase
{
public:
  typedef void (T::*AioAcceptCallback)(AioAccept<T,SKT> & aio);

  AioAcceptCallback m_callback;
  T *               m_obj;
  SKT          *    m_newskt;
  sockaddr_in       m_peer;

  AioAccept(int fd, T * obj, AioAcceptCallback callback,SKT * skt)
  {
    m_fd = fd;
    m_obj = obj;
    m_callback = callback;
    m_newskt   = skt;
    memset(&m_peer,0,sizeof m_peer);
  }

  void Invoke()
  {
    (m_obj->*m_callback)(*this);
  }

  int EndAccept()
  {
    socklen_t l = sizeof m_peer;
    return accept(m_fd,(sockaddr*)&m_peer,&l);
  }

};

template<class T> class AioConnect : public AioBase
{
public:
  typedef void (T::*ConnectCallback)(AioConnect<T> & aio);

  ConnectCallback m_callback;
  T * m_obj ;

  AioConnect(int fd, T * obj, ConnectCallback callback)
  {
    m_fd = fd;
    m_obj= obj;
    m_callback = callback;
  }

  void Invoke()
  {
    (m_obj->*m_callback)(*this);
  }

  int EndConnect()
  {
    int res ;
    int value ;
    socklen_t len = sizeof value ;
    res = getsockopt(m_fd, SOL_SOCKET, SO_ERROR,(char *)&value,&len);
    if (res == -1)
      value = errno;
    return res ;
  }
};

class CAsyncSocket : public CFileHandle
{
public:
								CAsyncSocket(CMsgQueue & q);
	virtual 			~CAsyncSocket();
	int						Create(int port,int type);
	int						Close();
	int						ShutDown(int how);
	int						Listen(int backlog);
	int						Bind(const struct sockaddr * sa,socklen_t salen);
	int						Bind(int port);

	int						GetLastError() { return m_lasterror;}
	int						SetOption(int name,int value);
	int						GetOption(int name,int & value);
	int						GetName(struct sockaddr * sa,socklen_t * salen);
	struct sockaddr_in m_peer;
private:
	void					SetLastError(int nerr) { m_lasterror = nerr;}
protected:
	int 			    m_type ;
	int				    m_port ;
	int 			    m_lasterror;
};
#endif /*ASYNCSOCKET_H_*/
