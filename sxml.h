#ifndef _SXML_H_INCLUDED
#define _SXML_H_INCLUDED

//---

typedef enum
{
	SXML_ERROR_XMLINVALID= -1,
	SXML_SUCCESS= 0,
	SXML_ERROR_BUFFERDRY= 1,
	SXML_ERROR_TOKENSFULL= 2,
} sxmlerr_t;

//---

typedef struct sxml_parser
{
	unsigned bufferpos;
	unsigned ntokens;
	unsigned taglevel;
} sxml_parser;

//---

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
	unsigned startpos;
	unsigned endpos;
} sxmltok_t;

//---

#ifdef __cplusplus
extern "C" {
#endif

void sxml_init(sxml_parser *parser);
sxmlerr_t sxml_parse(sxml_parser *parser, const char *buffer, unsigned bufferlen, sxmltok_t tokens[], unsigned num_tokens);

#ifdef __cplusplus
}
#endif

#endif //_SXML_H_INCLUDED