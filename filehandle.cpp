#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include "filehandle.h"
#include "mdmt.h"

class CFileHandleList
{
public:
	CFileHandleList();
	~CFileHandleList();
	void					Lock();
	void					Unlock();
	CFileHandle * GetFirst() { return m_first;}
	CFileHandle * GetLast()  { return m_last; }
	void					AddTail(CFileHandle * pfh);
	void					Remove(CFileHandle * pfh);
	int 					Find(CFileHandle * pfh);
	CFileHandle * FindFd(int fd);
	int						GetCount() { return m_count;}
private:
	CFileHandle * 	m_first;
	CFileHandle * 	m_last ;
	int							m_count;
	pthread_mutex_t m_lock;
};

static CFileHandleList filehandles ;
struct epoll_event CFileHandle::m_pollmap[256];
int CFileHandle::m_epollfd;

pthread_t fhworker;

CEvent FileHandleListEvent ;


void * CFileHandle::WorkerThread(void * p)
{
	int res ;
	CFileHandle * pfh ;
	if (m_epollfd == 0)
	{
		m_epollfd = epoll_create(sizeof m_pollmap/sizeof m_pollmap[0]);
		if (m_epollfd == -1)
		{
			perror("epoll_create");
			return NULL;
		}
	}
	while(1)
	{
		res = epoll_wait(m_epollfd,m_pollmap,sizeof m_pollmap/sizeof m_pollmap[0],-1);
		if (res > 0)
		{
			for (int i = 0 ; i < res ; i++)
			{
				struct epoll_event * pfd = m_pollmap+i ;
				if (pfd->events)
				{
					pfh = (CFileHandle*)pfd->data.ptr;
					if (pfh != NULL)
					{
						if (pfd->events & POLLHUP)
							pfh->OnPollHup();
						else if (pfd->events & POLLERR)
							pfh->OnPollErr();
						else
						{
							if (pfd->events & POLLIN)
								pfh->OnPollIn();
							if (pfd->events & POLLOUT)
								pfh->OnPollOut();
						}
					}
				}
			}
		}
	}
}

void InitFileHandleWorker()
{
	pthread_attr_t attr ;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,0x8000);
	pthread_create(&fhworker,&attr,CFileHandle::WorkerThread,NULL);
}

CFileHandleList::CFileHandleList()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&m_lock,&attr);
	m_first= NULL;
	m_last = NULL;
}

CFileHandleList::~CFileHandleList()
{
	pthread_mutex_destroy(&m_lock);
}

void CFileHandleList::Lock()
{
	pthread_mutex_lock(&m_lock);
}

void CFileHandleList::Unlock()
{
	pthread_mutex_unlock(&m_lock);
}

void CFileHandleList::AddTail(CFileHandle * pfh)
{
	Lock();
	if (fhworker == 0)
		InitFileHandleWorker();
	if (m_first == NULL)

		m_first = pfh ;
	else
	{
		m_last->m_next = pfh ;
		pfh->m_prev = m_last;
	}
	m_last = pfh ;
	m_count++;
	Unlock();
}

int CFileHandleList::Find(CFileHandle * pfh)
{
	int res = 0 ;
	CFileHandle * p ;
	Lock();
	for (p = GetFirst() ; p != NULL ; p = p->GetNext())
	{
		if (p == pfh)
		{
			res = 1 ;
			break ;
		}
	}
	Unlock();
	return res ;
}

CFileHandle * CFileHandleList::FindFd(int fd)
{
	CFileHandle * p = NULL;
	Lock();
	for (p = GetFirst() ; p != NULL ; p = p->GetNext())
	{
		if (p->m_fd == fd)
			break;
	}
	Unlock();
	return p ;
}

void CFileHandleList::Remove(CFileHandle * pfh)
{
	Lock();
	if (pfh == m_first)
		m_first = pfh->m_next;
	if (pfh == m_last)
		m_last = pfh->m_prev;
	if (pfh->m_next != NULL)
			pfh->m_next->m_prev = pfh->m_prev;
	if (pfh->m_prev != NULL)
			pfh->m_prev->m_next = pfh->m_next;
	m_count--;
	Unlock();
}

CFileHandle::CFileHandle()
{
	m_next = NULL;
	m_prev = NULL;
	m_fd = -1 ;
	m_epoll.events   = 0;
	m_epoll.data.ptr = NULL;
	filehandles.AddTail(this);
}

CFileHandle::~CFileHandle()
{
	Close();
	filehandles.Remove(this);
}

void CFileHandle::Attach(int fd,int events)
{
	m_fd = fd ;
	SetBlocking(0);
	EPollAdd(events);
}

int CFileHandle::GetFlags()
{
	return fcntl(m_fd,F_GETFL);
}

int CFileHandle::SetFlags(int flags)
{
	return fcntl(m_fd,F_SETFL,flags);
}

int CFileHandle::EPollDel()
{
	int res ;
	m_epoll.data.ptr = NULL;
	m_epoll.events   = 0;
	res = epoll_ctl(m_epollfd,EPOLL_CTL_DEL,m_fd,&m_epoll);
	return res ;
}

int CFileHandle::EPollAdd(int events)
{
	int res = 0 ;
	if (m_epoll.data.ptr == NULL)
	{
		m_epoll.data.ptr = this ;
		m_epoll.events   = events|EPOLLET;
		if (m_epollfd == 0)
		{
			m_epollfd = epoll_create(sizeof m_pollmap/sizeof m_pollmap[0]);
			if (m_epollfd == -1)
			{
				perror("epoll_create");
				return -1;
			}
		}
		res = epoll_ctl(m_epollfd,EPOLL_CTL_ADD,m_fd,&m_epoll);
	}
	return res ;
}

int CFileHandle::EPollMod(int events)
{
	m_epoll.events = events|EPOLLET;
	return epoll_ctl(m_epollfd,EPOLL_CTL_MOD,m_fd,&m_epoll);
}

void CFileHandle::TakeOver(CFileHandle * other)
{
	m_fd = other->m_fd;
	m_epoll.data.ptr = this;
	EPollMod(other->m_epoll.events);
	other->m_epoll.data.ptr = NULL;
	other->m_epoll.events = 0 ;
	other->m_fd = -1;
}

int	CFileHandle::Open(const char * name,int flags)
{
	int res = 0 ;
	res = open(name,flags);
	if (res != -1)
		Attach(res,0);
	return res ;
}

int	CFileHandle::Close()
{
	int res = 0 ;
	if (m_fd != -1)
	{
		EPollDel();
		res = close(m_fd);
		m_fd = -1 ;
	}
	return res ;
}

int	CFileHandle::Write(const void * data,size_t size)
{
	return write(m_fd,data,size);
}

int CFileHandle::Read(void * data,size_t size)
{
	return read(m_fd,data,size);
}

int CFileHandle::Ioctl(int request,void * argp)
{
	return ioctl(m_fd,request,argp);
}

void CFileHandle::SetPollMask(int mask)
{
	EPollMod(mask);
}

int CFileHandle::SetBlocking(int on)
{
	int val = GetFlags();
	if (val != -1)
	{
		if (on == 0)
			val |= O_NONBLOCK;
		else
			val &= ~O_NONBLOCK;
		val = SetFlags(val);
	}
	return val ;
}
