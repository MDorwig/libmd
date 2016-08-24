/*
 * msgqueue.h
 *
 *  Created on: 27.10.2013
 *      Author: dorwig
 */

#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include "itemlist.h"
#include "mdmt.h"

typedef void (*MsgQueueDelegate)(unsigned long p1,unsigned p2,unsigned p3);

class CMsg
{
public:
  CMsg();
  virtual ~CMsg() {}
  virtual void Invoke() = 0;
  CListItem        m_list;
};

class CMsgQueue
{
public:
	CMsgQueue();
	~CMsgQueue();
  CItemList m_list ;
  CMutex  m_lock;
  CEvent  m_notempty;
  void    DoWaitInvoke();
  void    DoInvoke();
  void    PostMessage(CMsg * item);
};


#endif /* MSGQUEUE_H_ */
