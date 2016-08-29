/*
 * list.h
 *
 *  Created on: 02.11.2013
 *      Author: dorwig
 */

#ifndef ITEMLIST_H_
#define ITEMLIST_H_

#include <stddef.h>
#include <assert.h>
#include "mdmt.h"

class CItemList;
class CListItem
{
public:
  CListItem * m_next ;
  CListItem * m_prev ;
  CItemList * m_list;

  CListItem()
  {
    Init();
  }

  void Init()
  {
    m_next = NULL;
    m_prev = NULL;
    m_list = NULL;
  }

  ~CListItem() ;
};

class CItemList
{
public:

  CListItem * m_first;
  CListItem * m_last;
  int         m_count;

  CItemList()
  {
    m_first = m_last = NULL;
    m_count = 0;
  }

  virtual ~CItemList()
  {

  }
  /*
   * Add item to end of list
   */
  virtual void AddTail(CListItem * t);
  /*
   * Add item b before item a
   */
  virtual void AddBefore(CListItem * a,CListItem * b);
  /*
   * Add item to begin of list
   */
  virtual void AddHead(CListItem * t);
  /*
   * Remove item from list
   */
  virtual void Remove(CListItem * t);

  CListItem *  GetHead()
  {
    return m_first;
  }

  bool isEmpty()
  {
    return m_count == 0 ;
  }

  CListItem *  GetTail()
  {
    return m_last;
  }

  unsigned Count() { return m_count;}
};

//#define fromitem(item,typ,field) item == NULL ? NULL : ((typ*)((char*)item-offsetof(typ,field)))
#define suboffset(item,typ,offset) item == NULL ? NULL : ((typ*)((char*)item-offset))
#define addoffset(elem,offset) ((CListItem*)(((char*)elem)+offset))

template <class ElementType, const int offset_of_listitem> class TypedItemList : public CItemList
{
public:
  virtual void AddTail(ElementType * t)
  {
    CItemList::AddTail(addoffset(t,offset_of_listitem));
  }

  virtual void AddBefore(ElementType * a,ElementType * b)
  {
    CItemList::AddBefore(addoffset(a,offset_of_listitem),addoffset(b,offset_of_listitem));
  }

  virtual void AddHead(ElementType * t)
  {
    CItemList::AddHead(addoffset(t,offset_of_listitem));
  }

  virtual void Remove(ElementType * t)
  {
    CItemList::Remove(addoffset(t,offset_of_listitem));
  }

  virtual ElementType *  GetHead()
  {
    return suboffset(m_first,ElementType,offset_of_listitem);
  }

  virtual ElementType *  GetTail()
  {
    return suboffset(m_last,ElementType,offset_of_listitem);
  }

  virtual ElementType *  GetNext(ElementType * e)
  {
    if (e == NULL)
      return NULL;
    return suboffset(addoffset(e,offset_of_listitem)->m_next,ElementType,offset_of_listitem);
  }

  virtual ElementType *  GetPrev(ElementType * e)
  {
    if (e == NULL)
      return NULL;
    return suboffset(addoffset(e,offset_of_listitem)->m_prev,ElementType,offset_of_listitem);
  }
};

template <class ElementType,const int offset_of_item>  class LockedTypedItemList : public TypedItemList<ElementType,offset_of_item>
{
  typedef TypedItemList<ElementType,offset_of_item> Base;
public:

  void Lock()
  {
    m_mtx.Lock();
  }

  void Unlock()
  {
    m_mtx.Release();
  }

  void AddTail(ElementType * e)
  {
    m_mtx.Lock();
    Base::AddTail(e);
    m_mtx.Release();
  }

  void AddBefore(ElementType * a,ElementType * b)
  {
    m_mtx.Lock();
    Base::AddBefore(a,b);
    m_mtx.Release();
  }

  void AddHead(ElementType * t)
  {
    m_mtx.Lock();
    Base::AddHead(t);
    m_mtx.Release();
  }

  void Remove(ElementType * e)
  {
    m_mtx.Lock();
    Base::Remove(e);
    m_mtx.Release();
  }
  CMutex m_mtx;
};


#define listforeach(item,list) for( item = (list).GetHead() ; item != NULL ; item = (list).GetNext(item))
#define listforeachrev(item,list) for(item = (list).GetTail() ; item != NULL ; item = (list).GetPrev(item))


#endif /* ITEMLIST_H_ */
