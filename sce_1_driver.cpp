/*
 * scedriver.cpp
 *
 *  Created on: 21.02.2015
 *      Author: dorwig
 */

#include "scebase.h"
#include "thread.h"


SceDriver * SceDriver::driver;

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
    m_msgqueue.PostMessage(m);
  }

  unsigned  sce_msgid;
};

static SceDriverThread * scedriverthread;

SceDriver::SceDriver()
{
  driver = this;
  scedriverthread = new SceDriverThread() ;
}

void SceDriver::SendSignal(SceMsg * msg)
{
  scedriverthread->SendSignal(msg);
}

void SceDriver::AddProcess(SceBase * proc)
{
  driver->procs.AddTail(proc);
}

void SceDriver::RemoveProcess(SceBase * proc)
{
  driver->procs.Remove(proc);
}

void SceDriver::SetTraceDelegate(TraceDelegate d,void * context)
{
  driver->tracefunc = d ;
  driver->tracecontext = context;
}

SceBase * SceDriver::Find(SceBase * val)
{
  SceBase * s;
  listforeach(s,driver->procs)
  {
    if (s == val)
      return val ;
  }
  return NULL;
}

SceBase * SceDriver::getFist()
{
	return driver->procs.GetHead();
}

SceBase * SceDriver::getNext(SceBase * b)
{
	return driver->procs.GetNext(b);
}

int  SceDriver::Trace(const char * fmt,...)
{
  int i = 0 ;
  if (driver->tracefunc != NULL)
  {
    va_list lst ;
    va_start(lst,fmt);
    i = driver->tracefunc(driver->tracecontext,fmt,lst);
    va_end(lst);
  }
  return i ;
}

int  SceDriver::Trace(const char * fmt,va_list lst)
{
  int i = 0 ;
  if (driver->tracefunc != NULL)
  {
    i = driver->tracefunc(driver->tracecontext,fmt,lst);
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
