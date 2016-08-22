#ifndef FILEHANDLE_H_
#define FILEHANDLE_H_

#include <pthread.h>
#include <unistd.h>
#include <sys/poll.h>
#include "msgqueue.h"

class AioBase : public CMsg
{
public:
  AioBase(void) { m_fd = -1;}
  virtual ~AioBase() { }
  int    m_fd ;
};

template<class T> class AioReadWrite : public AioBase
{
public:
  typedef void (T::*AioReadWriteCallback)(AioReadWrite<T> &);
  AioReadWriteCallback m_callback;
  T    * m_obj;
  char * m_buffer;
  size_t m_buflen;

  AioReadWrite(int fd, T * obj, AioReadWriteCallback callback, char * buf, size_t len)
  {
    m_fd = fd;
    m_obj= obj;
    m_callback = callback;
    m_buffer = buf;
    m_buflen = len;
  }

  void Invoke()
  {
    (m_obj->*m_callback)(*this);
  }

  int  EndRead() const
  {
    int status = read(m_fd,m_buffer,m_buflen);
    return status;
  }

  int EndWrite() const
  {
    int status ;
    status = write(m_fd,m_buffer,m_buflen);
    return status ;
  }
};

class CFileHandle
{
public:
								CFileHandle(CMsgQueue & q);
	virtual				~CFileHandle();
	void					Attach(int fd);
	int						GetHandle() { return m_fd;}
	int						Open(const char * name,int flags);
	int						Close();
	int						Ioctl(int request,void * argp);
	int						GetFlags();
	int						SetFlags(int value);
	int						SetBlocking(int on);
	void          TakeOver(CFileHandle * other);
	static void * WorkerThread(void * arg);
  CListItem     m_list;
  CMsgQueue   & m_msgqueue;
	int						m_fd ;
	friend class CFileHandleList;
	AioBase *     m_inreq;
	AioBase *     m_outreq;
};
#endif /*FILEHANDLE_H_*/
