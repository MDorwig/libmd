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

#define mbroffset(typ,mbr) (((long unsigned)(&((typ*)4)->mbr))-4)
#define fromitem(item,typ,list) (typ*)((char*)item-mbroffset(typ,list))

#define listforeach(item,list) for( item = (list).m_first ; item != NULL ; item = item->m_next)



#endif /* ITEMLIST_H_ */
