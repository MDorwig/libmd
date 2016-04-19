/*
 * msgqueue.cpp
 *
 *  Created on: 02.11.2013
 *      Author: dorwig
 */
#include <stdio.h>
#include "msgqueue.h"

CMsg::CMsg()
{
}

CMsg::CMsg(unsigned id,long unsigned p1,long unsigned p2,long unsigned p3)
{
  m_id = id ;
  m_p1 = p1 ;
  m_p2 = p2 ;
  m_p3 = p3 ;
  m_d  = NULL;
}

CMsg::CMsg(MsgQueueDelegate d,void * p1,long unsigned p2,long unsigned p3)
{
  m_id = 0;
  m_p1 = (long unsigned)p1 ;
  m_p2 = p2 ;
  m_p3 = p3 ;
  m_d  = d ;
}


CMsgQueue::CMsgQueue()
{
}

CMsgQueue::~CMsgQueue()
{
}

unsigned CMsgQueue::CreateMessageId()
{
  static unsigned nextmsgid ;
  return ++nextmsgid;
}

void CMsgQueue::PostMessage(CMsg * msg)
{
  m_lock.Lock();
  m_list.AddTail(*msg);
  m_notempty.Set();
  m_lock.Release();
}

void CMsgQueue::PostMessage(const CMsg & item)
{
  CMsg * msg = new CMsg(item);
  PostMessage(msg);
}

void CMsgQueue::PostMessage(unsigned id,long unsigned p1,long unsigned p2,long unsigned p3)
{
  CMsg * msg = new CMsg(id,p1,p2,p3);
  PostMessage(msg);
}

void CMsgQueue::PostMessage(MsgQueueDelegate d,void * pthis,long unsigned p2,long unsigned p3)
{
  CMsg * msg = new CMsg(d,pthis,p2,p3);
  PostMessage(msg);
}

void CMsgQueue::GetMessage(CMsg & msg)
{
  CMsg * m ;
  while(1)
  {
      m_lock.Lock();
      m = (CMsg*)m_list.GetHead();
      while (m == NULL)
      {
          m_lock.Release();
          m_notempty.Wait();
          m = (CMsg*)m_list.GetHead();
          m_lock.Lock();
      }
      m_list.Remove(*m);
      m_lock.Release();
      if (!m->isDelegate())
      {
          msg = *m ;
          delete m ;
          break ;
      }
      m->Invoke();
      delete m ;
  }
}
