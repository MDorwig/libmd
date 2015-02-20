/*
 * scebase.cpp
 *
 *  Created on: 20.02.2015
 *      Author: dorwig
 */

#include <string.h>
#include "scebase.h"
#include "msgqueue.h"
#include "mdmt.h"

int SceBase::nextpid;
CItemList SceDriver::procs;
SceDriver scedriver;
static SceBase   sceroot;
static CMsgQueue sceMsgQueue;

SceBase::SceBase()
{
	parent = NULL;
	name = strdup("root");
}

SceBase::SceBase(SceBase * fparent,const char * fname)
{
	parent = fparent;
	name   = strdup(fname);
	pid    = ++nextpid;
	SceDriver::procs.AddTail(list);
}

SceBase * SceBase::getRoot()
{
	return & sceroot;
}

void SceBase::setState(int st)
{
}

void SceBase::exitProcess()
{
}

int SceBase::Dispatch(SceMsg * msg)
{
	return 0 ;
}

SceBase::~SceBase()
{
	delete name ;
	SceDriver::procs.Remove(list);
}


void SceDriver::SendSignal(SceMsg * msg)
{
	CMsg * m = new CMsg(MSG_SCEMSG,(unsigned)msg);
	sceMsgQueue.PostMessage(*m);
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

void SceDriver::Run()
{
	CMsg msg ;
	sceMsgQueue.GetMessage(msg);
	if (msg.Id() == MSG_SCEMSG)
	{
		SceMsg * m = (SceMsg*)msg.P1();
		if (Find(m->to) != NULL)
		{
			SCE_TRACE("%10d Proess %s send signal %s to %s\n",getTickCount(),m->from->getName(),GetSignalName(m->id),m->to->getName());
			m->to->Dispatch(m);
			delete m ;
		}
	}
}
