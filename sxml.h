#ifndef _SXML_H_INCLUDED
#define _SXML_H_INCLUDED

typedef enum
{
	SXML_INSTRUCTION,
	SXML_DOCTYPE,
	SXML_STARTTAG,
	SXML_ENDTAG,
	SXML_COMMENT,
	SXML_CDATA,
	SXML_CHARACTER,	
} sxmltype_t;

typedef struct sxmltok_t
{
	unsigned short type;
	unsigned short size;
	unsigned start;
	unsigned end;
#ifdef SXML_PARENT_LINKS
	int parent;
#endif
} sxmltok_t;

//---

typedef enum
{
	SXML_ERROR_NOMEM = -1,
	SXML_ERROR_INVAL = -2,
	SXML_ERROR_PART = -3,
	SXML_SUCCESS = 0
} sxmlerr_t;

typedef struct sxml_parser
{
	unsigned pos;	//offset in the XML string
	unsigned toknext;	//next token to allocate
	unsigned indent;
#ifdef SXML_PARENT_LINKS
	int toksuper;	//superior token node, e.g parent object or array
#endif
} sxml_parser;

#define SXML_IGNORE_INSTRUCTION	0x1
#define	SXML_IGNORE_DOCTYPE	0x2
#define	SXML_IGNORE_COMMENT	0x4
#define	SXML_IGNORE_WHITESPACE	0x8

#ifdef __cplusplus
extern "C" {
#endif

void sxml_init(sxml_parser *parser);
sxmlerr_t sxml_parse(sxml_parser *parser, const char *xml, unsigned xmllen, sxmltok_t tokens[], unsigned num_tokens);

#ifdef __cplusplus
}
#endif

#endif //_SXML_H_INCLUDED