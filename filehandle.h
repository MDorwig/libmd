#ifndef FILEHANDLE_H_
#define FILEHANDLE_H_

#include <pthread.h>
#include <sys/epoll.h>

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
	int           GetPollMask() { return m_epoll.events & ~(EPOLLET);}
	int 					EPollDel();
	int 					EPollAdd(int events);
	int 					EPollMod(int events);
	CFileHandle * GetNext() { return m_next;}
	static void * WorkerThread(void * arg);
protected:
	CFileHandle	*	m_next;
	CFileHandle	*	m_prev;
	epoll_event 	m_epoll;
private:	
	int						m_fd ;
	friend class CFileHandleList;
	static int		m_epollfd;
	static struct epoll_event m_pollmap[256];
	
};
#endif /*FILEHANDLE_H_*/
