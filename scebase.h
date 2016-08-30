/*
 * scbase.h
 *
 *  Created on: 20.02.2015
 *      Author: dorwig
 */

#ifndef LIBMD_SCEBASE_H_
#define LIBMD_SCEBASE_H_

#include <stdarg.h>

#include "itemlist.h"

class SceMsg ;

class SceBase
{
public:
		SceBase(SceBase * par = NULL,const char * n = "root");
		virtual ~SceBase();

		virtual int getState() const
		{
			return state;
		}

		static SceBase * getRoot();

		void setState(int st);

		int getPid() const
		{
			return pid;
		}

		const char * getName()
		{
			return name;
		}

		virtual int Dispatch(SceMsg * pmsg);

		static int DispatchMessage(SceMsg * pmsg);

		virtual const char * getStateName(int st) const
		{
			return "" ;
		}

    virtual const char * getStateName() const
    {
      return getStateName(getState()) ;
    }

		virtual void exitProcess();

		virtual int Trace(const char * fmt,...) __attribute__((format (printf, 2, 3)));

		CListItem    list;

private:
		int 					state;
		SceBase * 		parent;
		const char * 	name ;
		int 					pid;
		static int 		nextpid;
};

typedef int (*TraceDelegate)(void * context,const char * fmt,va_list lst);

class SceDriver
{
public:
  SceDriver();

  ~SceDriver()
  {
  }
  static void SetTraceDelegate(TraceDelegate d,void * context);
  static void SendSignal(SceMsg * msg);
  static void OnTimer(int dt);
  static SceBase * Find(SceBase * val);
  static SceBase * getFist();
  static SceBase * getNext(SceBase * b);
  static void AddProcess(SceBase * proc);
  static void RemoveProcess(SceBase * proc);
  static void DispatchMsg(SceMsg * msg);
  static int  Trace(const char * fmt,...) __attribute__((format (printf, 1, 2)));
  static int  Trace(const char * fmt,va_list lst);
protected:
  LockedTypedItemList<SceBase,offsetof(SceBase,list)> procs ;
  CItemList timers;
  TraceDelegate tracefunc;
  void        * tracecontext;
  static SceDriver * driver;
};

class SceMsg
{
public:
	SceMsg(SceBase * src,SceBase * dst,int msgid)
	{
		from = src ;
		to   = dst ;
		id   = msgid;
		SceDriver::Trace("SceMsg %d created\n",id);
	}
	~SceMsg()
	{
		SceDriver::Trace("SceMsg %d destroyed\n",id);
	}
	SceBase * from;
	SceBase * to ;
	int       id ;
	int getId() { return id;}
};


class SceTimer
{
public:
	SceTimer(const char * n,void (*callback)(SceBase *),SceBase * p);

	~SceTimer()
	{
		Kill();
	}

	void operator = (int t);
	void Kill();
	void (*cb)(SceBase*);

	SceBase * 		proc;
	const char * 	name;
	int 					val ;
	CListItem 		list;
} ;

const char * GetSignalName(int id);

#endif /* LIBMD_SCEBASE_H_ */
