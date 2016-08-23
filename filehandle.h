#ifndef FILEHANDLE_H_
#define FILEHANDLE_H_

#include <pthread.h>
#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/poll.h>
#define EPOLLIN POLLIN
#define EPOLLOUT POLLOUT
#define EPOLLET  0
#endif

class CFileHandle
{
public:
								CFileHandle();
	virtual				~CFileHandle();
	void					Attach(int fd,int events);
	int						GetHandle() { return m_fd;}
	int						Open(const char * name,int flags);
	int						Close();
	int						Read(void * buffer,size_t size);
	int						Write(const void * data,size_t size);
	int						Ioctl(int request,void * argp);
	int						GetFlags();
	int						SetFlags(int value);
	int						SetBlocking(int on);
	virtual void 	OnPollErr()= 0;
	virtual void 	OnPollHup()= 0;
	virtual void 	OnPollIn() = 0;
	virtual void 	OnPollOut()= 0;
	void					SetPollMask(int mask);
#ifdef _SYS_EPOLL_H
	int           GetPollMask() { return m_epoll.events & ~(EPOLLET);}
#else
	int           GetPollMask() { return m_events;}
#endif
	int 					EPollDel();
	int 					EPollAdd(int events);
	int 					EPollMod(int events);
	void          TakeOver(CFileHandle * other);
	CFileHandle * GetNext() { return m_next;}
	static void * WorkerThread(void * arg);
	static void   InitEpoll();
protected:
	CFileHandle	*	m_next;
	CFileHandle	*	m_prev;
#ifdef _SYS_EPOLL_H
	epoll_event 	m_epoll;
#else
	unsigned      m_events;
#endif
private:
	int						m_fd ;
	friend class CFileHandleList;
#ifdef _SYS_EPOLL_H
	static int		m_epollfd;
	static struct epoll_event m_pollmap[256];
#else
	static struct pollfd m_pollmap[256];
#endif
};
#endif /*FILEHANDLE_H_*/
