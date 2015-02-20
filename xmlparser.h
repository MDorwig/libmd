/*
 * xmlparser.h
 *
 *  Created on: 31.08.2013
 *      Author: dorwig
 */

#ifndef XMLPARSER_H_
#define XMLPARSER_H_

#include <string>
#include <list>

using namespace std;

class xmlscanner
{
public:
  enum scannerstate
  {
    SC_ASCII,
    SC_UTF8,
  };

  xmlscanner();
  int nextchar(const char ** input);
  scannerstate m_state ;
  int          m_ch;
  int          m_utf8cnt;
};

class xmlAttr
{
public:
  xmlAttr(const string & name,const string & value)
  {
    m_name = name ;
    m_value= value;
  }
  string m_name ;
  string m_value;
};

#define for_each_attr(attr,node) \
	xmlAttrPtr attr; \
	for (xmlAttrIterator item = node->m_attr.begin() ;\
	item != node->m_attr.end() && (attr = *item) != NULL  ;\
	item++)


enum xmlNodeType
{
  XML_DECL_NODE,
  XML_ELEMENT_NODE,
};

class xmlparserException : public exception
{
public:
  xmlparserException(const char * fmt,...) __attribute__((format(printf,2,3)));
  ~xmlparserException() throw();
  const char *what() const throw();
  char * m_text;
};

class xmlNode;
class xmlAttr;

typedef xmlNode * xmlNodePtr ;
typedef xmlNode * xmlDocPtr ;
typedef xmlAttr * xmlAttrPtr ;
typedef char xmlChar ;

typedef list<xmlNodePtr>::iterator xmlNodeIterator;
typedef list<xmlAttrPtr>::iterator xmlAttrIterator;

class xmlNode
{
public:
  xmlNode(const string & name,xmlNodeType t)
  {
    m_name = name ;
    m_type = t ;
    m_parent = NULL;
  }
  ~xmlNode()
  {
  	for (xmlAttrIterator ai = m_attr.begin() ; ai != m_attr.end() ; ai++)
  	{
  		delete *ai ;
  	}
  	for (xmlNodeIterator ni = m_children.begin() ; ni != m_children.end() ; ni++)
  	{
  		delete *ni ;
  	}
  }

  bool hasChildren()  { return m_children.begin() != m_children.end() ; }
  bool hasAttributes(){ return m_attr.begin() != m_attr.end() ; }
  xmlNodeType m_type;
  xmlNode *   m_parent;
  string      m_name ;
  string      m_content;
  list<xmlAttr*> m_attr;
  list<xmlNode*> m_children;
};

#define for_each_child(child,node) \
	xmlNodePtr child ;\
	for (xmlNodeIterator item = node->m_children.begin() ;\
	     item != node->m_children.end() && (child = *item) != NULL ;\
	     item++)


class xmlparser
{
public:
  enum parserstate
  {
    PS_IDLE,
    PS_OPENTAG,
    PS_CLOSETAG,
    PS_START_TAG,
    PS_START_TAG1,
    PS_END_TAG,
    PS_IDENT_START,
    PS_IDENT,
    PS_STRING,
    PS_ESCAPE1,
    PS_ESCAPE2,
    PS_ESCAPE3,
    PS_ESCAPE4,
    PS_QUOT1,
    PS_QUOT2,
    PS_QUOT3,
    PS_QUOT4,
    PS_AMPAPOS,
    PS_AMP1,
    PS_AMP2,
    PS_APOS1,
    PS_APOS2,
    PS_APOS3,
    PS_LT,
    PS_GT,
    PS_CHARDATA,
    PS_XMLDECL,
    PS_COMMENT,
    PS_COMMENT1,
    PS_COMMENT2,
    PS_COMMENT3,
    PS_COMMENT4,
    PS_ATTR_LIST,
    PS_ATTR_NAME,
    PS_ATTR_EQ,
    PS_ATTR_VALUE,
    PS_COMPLETE,
  } ;

              	xmlparser(bool trace);
  virtual    	 ~xmlparser();
  bool        	isNameStartChar(int ch);
  bool        	isNameChar(int ch);
  bool        	isWhiteSpace(int ch);
  int         	parse(const char ** input);
  const char *	StateName(parserstate st);
  void        	setState(parserstate st);
  parserstate 	getState();
  void        	pushState(parserstate st);
  parserstate 	popState();
  void        	completeElement(const char * input);
  virtual void 	onXmlDecl(xmlNode * decl);
  virtual void 	onContent(xmlNode * node,const string & content);
  virtual void 	onElementComplete(xmlNode * node);
  virtual void 	onDocumentComplete(xmlNode * node);
  void        	AddNode(xmlNode * n);
  xmlscanner  	m_scanner;
  parserstate 	m_state[16] ;
  unsigned    	m_trace:1;
  unsigned    	m_skipws:1;
  int         	m_statesp;
  string 				m_ident;
  string 				m_attrval;
  int         	m_strdelim ;
  int         	m_charval;
  string 				m_lexval;
  xmlNode *   	m_root;
  xmlNode *   	m_curnode;
};





#endif /* XMLPARSER_H_ */
