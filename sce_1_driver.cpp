/*
 * scedriver.cpp
 *
 *  Created on: 21.02.2015
 *      Author: dorwig
 */

#include "scebase.h"
#include "thread.h"


class SceDriverThread : public CThread
{
public:
  SceDriverThread() : CThread("scedriver")
  {
    sce_msgid = CMsgQueue::CreateMessageId();
    Create();
  }

  void Main()
  {
    while(1)
    {
      CMsg msg ;
      m_msgqueue.GetMessage(msg);
      if (msg.Id() == sce_msgid)
      {
        SceDriver::DispatchMsg((SceMsg*)msg.P1());
      }
    }
  }

  void SendSignal(SceMsg * msg)
  {
    CMsg * m = new CMsg(sce_msgid,(long unsigned)msg);
    m_msgqueue.PostMessage(*m);
  }

  unsigned  sce_msgid;
};

static SceDriverThread * scedriverthread;

SceDriver::SceDriver()
{
  scedriverthread = new SceDriverThread() ;
}

void SceDriver::SendSignal(SceMsg * msg)
{
  scedriverthread->SendSignal(msg);
}

void SceDriver::AddProcess(SceBase * proc)
{
  driver.procs.AddTail(proc->list);
}

void SceDriver::RemoveProcess(SceBase * proc)
{
  driver.procs.Remove(proc->list);
}

void SceDriver::SetTraceDelegate(TraceDelegate d,void * context)
{
  driver.tracefunc = d ;
  driver.tracecontext = context;
}

SceBase * SceDriver::Find(SceBase * val)
{
  CListItem * item ;
  listforeach(item,(&driver.procs.m_list))
  {
    SceBase * s = fromitem(item,SceBase,list);
    if (s == val)
      return val ;
  }
  return NULL;
}

SceBase * SceDriver::getFist()
{
	SceBase * b = NULL;
	CListItem * item = driver.procs.GetHead();
	if (item != NULL)
	{
		b = fromitem(item,SceBase,list);
	}
	return b ;
}

SceBase * SceDriver::getNext(SceBase * b)
{
	SceBase * n = NULL;
	CListItem * item = driver.procs.GetNext(&b->list);
	if (item != NULL)
		n = fromitem(item,SceBase,list);
	return n ;
}

int  SceDriver::Trace(const char * fmt,...)
{
  int i = 0 ;
  if (driver.tracefunc != NULL)
  {
    va_list lst ;
    va_start(lst,fmt);
    i = driver.tracefunc(driver.tracecontext,fmt,lst);
    va_end(lst);
  }
  return i ;
}

int  SceDriver::Trace(const char * fmt,va_list lst)
{
  int i = 0 ;
  if (driver.tracefunc != NULL)
  {
    i = driver.tracefunc(driver.tracecontext,fmt,lst);
  }
  return i ;
}


void SceDriver::DispatchMsg(SceMsg * m)
{
  if (Find(m->to) != NULL)
  {
    Trace("%10d Process %s(%d) sends signal %s to Process %s(%d)\n",
        getTickCount(),
        m->from->getName(),
        m->from->getPid(),
        GetSignalName(m->id),
        m->to->getName(),
        m->to->getPid());

    if (m->to->Dispatch(m) == 0)
    {
      Trace("%10d Process %s(%d) did not handle signal %s in state %s\n",
          getTickCount(),
          m->to->getName(),
          m->to->getPid(),
          GetSignalName(m->id),
          m->to->getStateName());
    }
    delete m ;
  }
}
