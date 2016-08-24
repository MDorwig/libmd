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

CMsgQueue::CMsgQueue()
{
}

CMsgQueue::~CMsgQueue()
{
}

void CMsgQueue::PostMessage(CMsg * msg)
{
  m_lock.Lock();
  m_list.AddTail(&msg->m_list);
  m_notempty.Set();
  m_lock.Release();
}

void CMsgQueue::DoInvoke()
{
  CMsg * m = NULL;
  m_lock.Lock();
  CListItem * i = m_list.GetHead();
  if (i != NULL)
  {
    m_list.Remove(i);
    m_lock.Release();
    m = fromitem(i,CMsg,m_list);
    m->Invoke();
  }
  else
    m_lock.Release();
}

void CMsgQueue::DoWaitInvoke()
{
  CMsg * m ;
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

