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

static CItemList procs ;
static CItemList timers;
static TraceDelegate tracefunc;
static void        * tracecontext;
static SceDriver   scedriver;
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
  procs.AddTail(proc->list);
}

void SceDriver::RemoveProcess(SceBase * proc)
{
  procs.Remove(proc->list);
}

void SceDriver::SetTraceDelegate(TraceDelegate d,void * context)
{
  tracefunc = d ;
  tracecontext = context;
}

SceBase * SceDriver::Find(SceBase * val)
{
  CListItem * item ;
  listforeach(item,(&procs.m_list))
  {
    SceBase * s = fromitem(item,SceBase,list);
    if (s == val)
      return val ;
  }
  return NULL;
}

int  SceDriver::Trace(const char * fmt,...)
{
  int i = 0 ;
  if (tracefunc != NULL)
  {
    va_list lst ;
    va_start(lst,fmt);
    i = tracefunc(tracecontext,fmt,lst);
    va_end(lst);
  }
  return i ;
}

int  SceDriver::Trace(const char * fmt,va_list lst)
{
  int i = 0 ;
  if (tracefunc != NULL)
  {
    i = tracefunc(tracecontext,fmt,lst);
  }
  return i ;
}


void SceDriver::DispatchMsg(SceMsg * m)
{
  if (Find(m->to) != NULL)
  {
    Trace("%10d Proess %s send signal %s to %s\n",getTickCount(),m->from->getName(),GetSignalName(m->id),m->to->getName());
    m->to->Dispatch(m);
    delete m ;
  }
}
