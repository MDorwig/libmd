#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <assert.h>
#include "filehandle.h"
#include "mdmt.h"

class CFileHandleList : public CItemList
{
public:
	CFileHandleList();
	~CFileHandleList();
	void					Lock();
	void					Unlock();
	void					AddTail(CFileHandle * pfh);
	void					Remove(CFileHandle * pfh);
	int 					Find(CFileHandle * pfh);
	CFileHandle * FindFd(int fd);
	int						GetCount() { return m_count;}
private:
	pthread_mutex_t m_lock;
};

static CFileHandleList filehandles ;

pthread_t fhworker;

CEvent FileHandleListEvent ;


void * CFileHandle::WorkerThread(void * p)
{
  int nfd = 1 ;
  pollfd * pfd = (pollfd*)calloc(1,sizeof(pollfd));

  while(1)
  {
    if (nfd != filehandles.GetCount())
    {
      free(pfd);
      nfd = filehandles.GetCount();
      pfd = (pollfd*)calloc(nfd,sizeof (pollfd));
    }
    int i = 0;
    CListItem * item ;
    listforeach(item,filehandles)
    {
      CFileHandle * fh = fromitem(item,CFileHandle,m_list);
      if (fh->m_fd != -1)
      {
        pollfd * p = pfd+i;
        p->fd = fh->m_fd;
        p->events = 0 ;
        if (fh->m_inreq != NULL)
          p->events |= POLLIN;
        if (fh->m_outreq != NULL)
          p->events |= POLLOUT;
        if (p->events != 0)
          i++;
      }
    }
    fflush(stdout);
    int res = poll(pfd,i,100);
    if (res > 0)
    {
      for (int i = 0 ; i < nfd ; i++)
      {
        CFileHandle * fh = filehandles.FindFd(pfd[i].fd);
        if (fh != NULL)
        {
          AioBase * aio;
          if (pfd[i].revents & POLLIN)
          {
            aio = fh->m_inreq;
            fh->m_inreq = NULL;
            if (aio != NULL)
            {
              fh->m_msgqueue.PostMessage(aio);
            }
          }
          if (pfd[i].revents & POLLOUT)
          {
            aio = fh->m_outreq;
            fh->m_outreq = NULL;
            if (aio != NULL)
            {
              fh->m_msgqueue.PostMessage(aio);
            }
          }
        }
      }
    }
  }
	return NULL;
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
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
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
	CItemList::AddTail(&pfh->m_list);
	Unlock();
}

int CFileHandleList::Find(CFileHandle * pfh)
{
	int res = 0 ;
	CListItem * item ;
	Lock();
	listforeach(item,(*this))
	{
		if (item == &pfh->m_list)
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
  CListItem * item ;
	Lock();
	listforeach(item,(*this))
	{
	  CFileHandle* p = fromitem(item,CFileHandle,m_list);
		if (p->m_fd == fd)
		{
		  Unlock();
		  return p ;
		}
	}
	Unlock();
	return NULL ;
}

void CFileHandleList::Remove(CFileHandle * pfh)
{
	Lock();
	CItemList::Remove(&pfh->m_list);
	Unlock();
}

CFileHandle::CFileHandle(CMsgQueue & q) : m_msgqueue(q)
{
	m_fd = -1 ;
	filehandles.AddTail(this);
}

CFileHandle::~CFileHandle()
{
	Close();
	filehandles.Remove(this);
}

void CFileHandle::Attach(int fd)
{
	m_fd = fd ;
	SetBlocking(0);
}

int CFileHandle::GetFlags()
{
	return fcntl(m_fd,F_GETFL);
}

int CFileHandle::SetFlags(int flags)
{
	return fcntl(m_fd,F_SETFL,flags);
}

void CFileHandle::TakeOver(CFileHandle * other)
{
	m_fd = other->m_fd;
	other->m_fd = -1;
}

int	CFileHandle::Open(const char * name,int flags)
{
	int res = 0 ;
	res = open(name,flags);
	if (res != -1)
		Attach(res);
	return res ;
}

int	CFileHandle::Close()
{
	int res = 0 ;
	if (filehandles.Find(this))
	  filehandles.Remove(this);
	if (m_fd != -1)
	{
		res = close(m_fd);
		m_fd = -1 ;
	}
	return res ;
}

int CFileHandle::Ioctl(int request,void * argp)
{
	return ioctl(m_fd,request,argp);
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
