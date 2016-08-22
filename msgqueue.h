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
  CMsg(unsigned id, long unsigned p1 = 0, long unsigned p2 =0, long unsigned p3 = 0);
  virtual ~CMsg() {}
  unsigned      Id() const { return m_id;}
  long unsigned P1() const { return m_p1;}
  long unsigned P2() const { return m_p2;}
  long unsigned P3() const { return m_p3;}
  virtual void Invoke() = 0;
  CListItem        m_list;
  unsigned         m_id ;
  long unsigned    m_p1 ;
  long unsigned    m_p2 ;
  long unsigned    m_p3 ;
};

class CMsgQueue
{
public:
	CMsgQueue();
	~CMsgQueue();
  CItemList m_list ;
  CMutex  m_lock;
  CEvent  m_notempty;
  CMsg * GetMessage();
  CMsg * PeekMessage();
  void PostMessage(CMsg * item);
  static unsigned CreateMessageId();
private:
};


#endif /* MSGQUEUE_H_ */
