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

static SceBase   sceroot;

#if 0
SceBase::SceBase() : SceBase(NULL,"root")
{
	parent = NULL;
	name = strdup("root");
}
#endif

SceBase::SceBase(SceBase * fparent,const char * fname)
{
	parent = fparent;
	name   = strdup(fname);
	pid    = ++nextpid;
	SceDriver::AddProcess(this);
}

SceBase * SceBase::getRoot()
{
	return & sceroot;
}

void SceBase::setState(int st)
{
  SceDriver::Trace("Process %s Transition from %s to %s\n",getName(),getStateName(),getStateName(st));
  state = st ;
}

void SceBase::exitProcess()
{
}

int SceBase::Dispatch(SceMsg * msg)
{
	return 0 ;
}

int SceBase::Trace(const char * fmt,...)
{
  int n ;
  va_list lst ;
  va_start(lst,fmt);
  n = SceDriver::Trace(fmt,lst);
  va_end(lst);
  return n ;
}

SceBase::~SceBase()
{
  SceDriver::RemoveProcess(this);
	delete name ;
}

