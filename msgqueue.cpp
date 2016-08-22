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
  m_list.AddTail(&msg->m_list);
  m_notempty.Set();
  m_lock.Release();
}

CMsg * CMsgQueue::PeekMessage()
{
  CMsg * m = NULL;
  CListItem * i = m_list.GetHead();
  if (i != NULL)
    m = fromitem(i,CMsg,m_list);
  return m ;
}

CMsg * CMsgQueue::GetMessage()
{
  CMsg * m ;
  while(1)
  {
      CListItem * item ;
      m_lock.Lock();
      item = m_list.GetHead();
      while (item == NULL)
      {
          m_lock.Release();
          m_notempty.Wait();
          m_lock.Lock();
          item = m_list.GetHead();
      }
      m_list.Remove(item);
      m_lock.Release();
      m = fromitem(item,CMsg,m_list);
      m->Invoke();
      delete m ;
  }
}

