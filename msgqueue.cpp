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
  Clear();
}

CMsg::CMsg(unsigned id,long unsigned p1,long unsigned p2,long unsigned p3)
{
  m_id = id ;
  m_p1 = p1 ;
  m_p2 = p2 ;
  m_p3 = p3 ;
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


void CMsgQueue::PostMessage(unsigned id,long unsigned p1,long unsigned p2,long unsigned p3)
{
  CMsg * msg = new CMsg(id,p1,p2,p3);
  PostMessage(msg);
}

void CMsgQueue::GetMessage(CMsg & msg)
{
  CMsg * m ;
  m_lock.Lock();
  m = (CMsg*)m_list.GetHead();
  while (m == NULL)
  {
      m_lock.Release();
      m_notempty.Wait();
      m_lock.Lock();
      m = (CMsg*)m_list.GetHead();
  }
  m_list.Remove(*m);
  m_lock.Release();
  if (m->Invoke())
  {
    msg.Clear();
  }
  else
  {
    msg = *m ;
  }
  delete m ;
}

bool CMsgQueue::PeekMessage(CMsg & msg)
{
  bool res = false;
  m_lock.Lock();
  CMsg * m = (CMsg*)m_list.GetHead();
  if (m != NULL)
  {
    m_list.Remove(*m);
    m_lock.Release();
    if (!m->Invoke())
    {
      msg = *m ;
    }
    else
    {
      m->Invoke();
    }
    delete m ;
    res = true;
  }
  else
  {
    m_lock.Release();
  }
  return res ;
}
