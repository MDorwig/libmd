/*
 * scbase.h
 *
 *  Created on: 20.02.2015
 *      Author: dorwig
 */

#ifndef LIBMD_SCEBASE_H_
#define LIBMD_SCEBASE_H_


#include "list.h"
#include "scetrace.h"

class SceMsg ;

class SceBase
{
public:
		SceBase() ;

		SceBase(SceBase * par,const char * n);
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

		virtual void exitProcess();

		CListItem    list;

private:
		int 					state;
		SceBase * 		parent;
		const char * 	name ;
		int 					pid;
		static int 		nextpid;
};

class SceMsg
{
public:
	SceMsg(SceBase * src,SceBase * dst,int msgid)
	{
		from = src ;
		to   = dst ;
		id   = msgid;
		SCE_TRACE("SceMsg %d created\n",id);
	}
	~SceMsg()
	{
		SCE_TRACE("SceMsg %d destroyed\n",id);
	}
	SceBase * from;
	SceBase * to ;
	int       id ;
	int getId() { return id;}
};


class SceDriver
{
public:
	SceDriver()
	{
	}

	~SceDriver()
	{
	}

	static void SendSignal(SceMsg * msg);
	static void OnTimer(int dt);
	static SceBase * Find(SceBase * val);
	static void Run(void);
	static CItemList procs ;
	static CItemList timers;
	static SceBase * sceroot;
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

#include "model/signals.h"

#endif /* LIBMD_SCEBASE_H_ */
