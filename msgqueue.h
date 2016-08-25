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

class CMsg : public CListItem
{
public:
  CMsg();
  CMsg(unsigned id, long unsigned p1 = 0, long unsigned p2 =0, long unsigned p3 = 0);
  void Clear()
  {
    m_id = ~0u ;
    m_p1 = 0 ;
    m_p2 = 0 ;
    m_p3 = 0 ;
    m_p3 = 0 ;
  }
  virtual ~CMsg() {}
  unsigned      Id() const { return m_id;}
  long unsigned P1() const { return m_p1;}
  long unsigned P2() const { return m_p2;}
  long unsigned P3() const { return m_p3;}

  virtual bool Invoke()
  {
    return false;
  }

private:
  unsigned         m_id ;
  long unsigned    m_p1 ;
  long unsigned    m_p2 ;
  long unsigned    m_p3 ;
};

template<class cls> class MsgDelegate0 : public CMsg
{
  typedef void (cls::*callback)(void);

  bool Invoke()
  {
    (m_obj->*m_callback)();
    return true;
  }

  cls * m_obj ;
  callback m_callback;
public:
  MsgDelegate0(cls * obj, callback invoke_me)
  {
    m_obj = obj ;
    m_callback = invoke_me;
  }
};

template<class cls,typename arg> class MsgDelegate1 : public CMsg
{
  typedef void (cls::*callback)(arg);

  bool Invoke()
  {
    (m_obj->*m_callback)(m_arg);
    return true;
  }

  cls * m_obj ;
  arg   m_arg;
  callback m_callback;
public:
  MsgDelegate1(cls * obj, callback invoke_me,arg a)
  {
    m_obj = obj ;
    m_arg = a ;
    m_callback = invoke_me;
  }
};

template<class cls,typename arg1,typename arg2> class MsgDelegate2 : public CMsg
{
  typedef void (cls::*callback)(arg1,arg2);

  bool Invoke()
  {
    (m_obj->*m_callback)(m_arg1,m_arg2);
    return true;
  }

  cls * m_obj ;
  arg1  m_arg1;
  arg2  m_arg2;
  callback m_callback;
public:
  MsgDelegate2(cls * obj, callback invoke_me,arg1 a1,arg2 a2)
  {
    m_obj = obj ;
    m_arg1 = a1 ;
    m_arg2 = a2 ;
    m_callback = invoke_me;
  }

};

class CMsgQueue
{
public:
	CMsgQueue();
	~CMsgQueue();
  CItemList m_list ;
  CMutex    m_lock;
  CEvent    m_notempty;
  void PostMessage(unsigned id, long unsigned p1 = 0, long unsigned p2 = 0, long unsigned p3 = 0);
  void PostMessage(CMsg * item);
  void GetMessage(CMsg & item);
  bool PeekMessage(CMsg & item);
  static unsigned CreateMessageId();
private:
};


#endif /* MSGQUEUE_H_ */
