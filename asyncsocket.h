#ifndef ASYNCSOCKET_H_
#define ASYNCSOCKET_H_

#include "filehandle.h"
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
enum sktstates
{
	SKT_IDLE,
	SKT_LISTEN,
	SKT_BOUND,
	SKT_CONNECTING,
	SKT_CONNECTED,
	SKT_CLOSED
};

#define	FD_READ 		1
#define	FD_WRITE 		2
#define FD_OOB 			4
#define	FD_ACCEPT 	8
#define FD_CONNECT 	16
#define FD_CLOSE		32

class CAsyncSocket : public CFileHandle
{
public:	
								CAsyncSocket();
	virtual 			~CAsyncSocket();
	int						Create(int port,int type,int events = FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE);
	void					AsyncSelect(int events);
	void					OnPollErr();
	void					OnPollHup();
	void					OnPollIn();
	void					OnPollOut();
	virtual void  OnEvent(int event,int nerr);
	virtual	void 	OnConnect(int nerr);	
	virtual	void 	OnAccept(int nerr);	
	virtual	void 	OnClose(int nerr);
	virtual void 	OnReceive(int nerr);
	virtual void 	OnSend(int nErrorCode);
	int						Close();
	int						ShutDown(int how);
	void					Dispatch(int events,int nerr);
	int						Listen(int backlog);
	int						Bind(const struct sockaddr * sa,socklen_t salen);
	int						Accept(CAsyncSocket & accskt,struct sockaddr * sa,socklen_t * salen);
	int						Connect(struct sockaddr * sa,socklen_t salen);
	int						Connect(const char * host,int port);
	int						Receive(void * buf,size_t len,int flags = 0);
	int						Send(const void * buf,size_t len,int flags = 0);
	int						GetLastError() { return m_lasterror;}
	int						SetOption(int name,int value);
	int						GetOption(int name,int & value);
private:	
	void					SetLastError(int nerr) { m_lasterror = nerr;}
protected:	
	int 			m_type ;
	int				m_port ;
	int 			m_lasterror;
	int				m_eventmask;
private:
	sktstates m_state;
};
#endif /*ASYNCSOCKET_H_*/
