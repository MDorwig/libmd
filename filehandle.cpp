#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#if defined __linux__ || defined __CYGWIN__
#include <sys/ioctl.h>
#include <poll.h>
#endif
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
#ifdef _SYS_EPOLL_H
struct epoll_event CFileHandle::m_pollmap[256];
int CFileHandle::m_epollfd;
#else
struct pollfd CFileHandle::m_pollmap[256];
#endif

pthread_t fhworker;

CEvent FileHandleListEvent ;

void * CFileHandle::WorkerThread(void * p)
{
	int res ;
	CFileHandle * pfh ;
	while(1)
	{
#ifdef _SYS_EPOLL_H
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
						if (pfd->events & EPOLLHUP)
						{
							pfh->OnPollHup();
						}
						else if (pfd->events & EPOLLERR)
						{
							pfh->OnPollErr();
					  }
						else
						{
							if (pfd->events & EPOLLIN)
							{
								pfh->OnPollIn();
							}
							if (pfd->events & EPOLLOUT)
							{
								pfh->OnPollOut();
              }
						}
					}
				}
			}
		}
#else
		size_t npoll = 0 ;
		for (pfh = filehandles.GetFirst() ; pfh != NULL && npoll < sizeof(m_pollmap)/sizeof m_pollmap[0]; pfh = pfh->GetNext())
		{
			if (pfh->m_fd != -1 && pfh->m_events != 0)
			{
				pollfd & pfd = m_pollmap[npoll];
				pfd.fd = pfh->m_fd;
				pfd.events = pfh->m_events;
				pfd.revents= 0;
				npoll++;
			}
		}
		res = poll(m_pollmap,npoll,100);
		if (res > 0)
		{
			for (size_t i = 0 ; i < npoll ; i++)
			{
				pollfd & pfd = m_pollmap[i] ;
				if (pfd.revents == 0) continue;
				pfh = filehandles.FindFd(pfd.fd);
				if (pfh != NULL)
				{
					if (pfd.revents & POLLERR)
						pfh->OnPollErr();
					if (pfd.revents & POLLHUP)
						pfh->OnPollHup();
					if (pfd.revents & POLLIN)
					{
						pfh->m_events &= ~POLLIN;
						pfh->OnPollIn();
					}
					if (pfd.revents & POLLOUT)
					{
						pfh->m_events &= ~POLLOUT;
						pfh->OnPollOut();
					}
				}
			}

		}
#endif
	}
	return NULL;
}

void InitFileHandleWorker()
{
	pthread_attr_t attr ;

	CFileHandle::InitEpoll();
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
	{
		m_first = pfh ;
	}
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
#ifdef _SYS_EPOLL_H
	m_epoll.events   = 0;
	m_epoll.data.ptr = NULL;
#else
	m_events = 0 ;
#endif
	filehandles.AddTail(this);
}

CFileHandle::~CFileHandle()
{
	Close();
	filehandles.Remove(this);
}

void CFileHandle::InitEpoll()
{
#ifdef __linux__
  if (m_epollfd == 0)
  {
    m_epollfd = epoll_create(sizeof m_pollmap/sizeof m_pollmap[0]);
    assert (m_epollfd != -1);
  }
#endif
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
	int res = 0;
#ifdef _SYS_EPOLL_H
	m_epoll.data.ptr = NULL;
	m_epoll.events   = 0;
	res = epoll_ctl(m_epollfd,EPOLL_CTL_DEL,m_fd,&m_epoll);
#else
	m_events = 0 ;
#endif
	return res ;
}

int CFileHandle::EPollAdd(int events)
{
	int res = 0 ;
#ifdef _SYS_EPOLL_H
	if (m_epoll.data.ptr == NULL)
	{
		m_epoll.data.ptr = this ;
		m_epoll.events   = events|EPOLLET;
		res = epoll_ctl(m_epollfd,EPOLL_CTL_ADD,m_fd,&m_epoll);
	}
#else
	m_events = events;
#endif
	return res ;
}

int CFileHandle::EPollMod(int events)
{
#ifdef _SYS_EPOLL_H
	m_epoll.events = events|EPOLLET;
	return epoll_ctl(m_epollfd,EPOLL_CTL_MOD,m_fd,&m_epoll);
#else
	m_events |= events;
	return 0;
#endif
}

void CFileHandle::TakeOver(CFileHandle * other)
{
#ifdef _SYS_EPOLL_H
	m_fd = other->m_fd;
	m_epoll.data.ptr = this;
	EPollMod(other->m_epoll.events);
	other->m_epoll.data.ptr = NULL;
	other->m_epoll.events = 0 ;
	other->m_fd = -1;
#else
	m_fd = other->m_fd;
	m_events = other->m_events;
	other->m_fd = -1;
	other->m_events = 0 ;
#endif
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
	int res ;
	res = write(m_fd,data,size);
#ifndef _SYS_EPOLL_H
  SetPollMask(GetPollMask()|EPOLLIN);
#endif
	return res ;
}

int CFileHandle::Read(void * data,size_t size)
{
	int n = read(m_fd,data,size);
#ifdef _SYS_EPOLL_H
	if (n != -1)
	  SetPollMask(GetPollMask()|EPOLLIN);
#else
  SetPollMask(GetPollMask()|EPOLLIN);
#endif
	return n ;
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
