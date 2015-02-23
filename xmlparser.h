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

typedef enum {
    XML_ELEMENT_NODE=   1,
    XML_ATTRIBUTE_NODE=   2,
    XML_TEXT_NODE=    3,
    XML_CDATA_SECTION_NODE= 4,
    XML_ENTITY_REF_NODE=  5,
    XML_ENTITY_NODE=    6,
    XML_PI_NODE=    7,
    XML_COMMENT_NODE=   8,
    XML_DOCUMENT_NODE=    9,
    XML_DOCUMENT_TYPE_NODE= 10,
    XML_DOCUMENT_FRAG_NODE= 11,
    XML_NOTATION_NODE=    12,
    XML_HTML_DOCUMENT_NODE= 13,
    XML_DTD_NODE=   14,
    XML_ELEMENT_DECL=   15,
    XML_ATTRIBUTE_DECL=   16,
    XML_ENTITY_DECL=    17,
    XML_NAMESPACE_DECL=   18,
    XML_XINCLUDE_START=   19,
    XML_XINCLUDE_END=   20
} xmlElementType;


typedef enum {
    XML_ATTRIBUTE_CDATA = 1,
    XML_ATTRIBUTE_ID,
    XML_ATTRIBUTE_IDREF ,
    XML_ATTRIBUTE_IDREFS,
    XML_ATTRIBUTE_ENTITY,
    XML_ATTRIBUTE_ENTITIES,
    XML_ATTRIBUTE_NMTOKEN,
    XML_ATTRIBUTE_NMTOKENS,
    XML_ATTRIBUTE_ENUMERATION,
    XML_ATTRIBUTE_NOTATION
} xmlAttributeType;

typedef xmlElementType xmlNsType;

class xmlparserException : public exception
{
public:
  xmlparserException(const char * fmt,...) __attribute__((format(printf,2,3)));
  ~xmlparserException() throw();
  const char *what() const throw();
  char * m_text;
};

typedef unsigned char xmlChar;

typedef struct xmlDoc  *xmlDocPtr;
typedef struct xmlNode *xmlNodePtr;
typedef struct xmlAttr *xmlAttrPtr;
struct xmlAttr {
    xmlAttr(const char * name,const char * value);
    ~xmlAttr();
    xmlElementType   type;
    const xmlChar   *name;
    xmlNodePtr children;
    xmlNodePtr last;
    xmlNodePtr parent;
    xmlAttrPtr next;
    xmlAttrPtr prev;
    xmlDocPtr  doc;
    xmlAttributeType atype;
};


typedef class xmlNode *xmlNodePtr;
struct xmlNode {
    xmlNode(xmlElementType nt,const char * name);
    ~xmlNode();
    void AddAttr(xmlAttrPtr attr);
    void AddChild(xmlNodePtr child);
    void          * _private;
    xmlElementType  type;
    const xmlChar * name;
    xmlNodePtr      children;
    xmlNodePtr      last;
    xmlNodePtr      parent;
    xmlNodePtr      next;
    xmlNodePtr      prev;
    xmlDocPtr       doc;
    xmlChar       * content;
    xmlAttrPtr      properties;
};

typedef xmlDoc *xmlDocPtr;
struct xmlDoc {
    xmlDoc(const char * name);
    ~xmlDoc();
    void AddChild(xmlNode * n);
    void          * _private; /* application data */
    xmlElementType  type;       /* XML_DOCUMENT_NODE, must be second ! */
    char          * name; /* name/filename/URI of the document */
    xmlNodePtr      children;  /* the document tree */
    xmlNodePtr      last;  /* last child link */
    xmlNodePtr      parent;  /* child->parent link */
    xmlNodePtr      next;  /* next sibling link  */
    xmlNodePtr      prev;  /* previous sibling link  */

    const xmlChar  *version;  /* the XML version string */
    const xmlChar  *encoding;   /* external initial encoding, if any */
};

#define BAD_CAST (xmlChar *)
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
  void        	completeElement(bool checkname,const char * input);
  virtual void 	onContent(xmlNodePtr node,const string & content);
  virtual void 	onElementComplete(xmlNodePtr node);
  virtual xmlDocPtr	onDocumentComplete(xmlDocPtr doc);
  /*
   * returnvalue : if onDocumentComplete returns NULL the current xmlDoc is not deleted
   *               if onDocumentComplete returns doc  the xmlDoc is deleted
   */
  void        	AddNode(xmlNodePtr n);
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
  xmlDocPtr   	m_doc;
  xmlNodePtr  	m_curnode;
};





#endif /* XMLPARSER_H_ */
