#ifndef _SXML_H_INCLUDED
#define _SXML_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*
 SXML
 short description of how to use SXML for parsing XML text

 SXML is a lightweight XML parser with no dependencies
 for parsing XML text you only need to call one function, sxml_parse().
 the function has the following return codes
*/

typedef enum
{
	SXML_ERROR_XMLINVALID= -1,	/* parser found invalid XML data - not much you can do beyond error reporting */
	SXML_SUCCESS= 0,			/* parser has completed successfully - parsing of XML document is complete */
	SXML_ERROR_BUFFERDRY= 1,	/* parser ran out of buffer data - refill buffer with more XML text to continue parsing */
	SXML_ERROR_TOKENSFULL= 2	/* parser has used all supplied tokens - provide more tokens for further output */
} sxmlerr_t;

typedef	struct sxml_t sxml_t;
typedef	struct sxmltok_t sxmltok_t;
sxmlerr_t sxml_parse(sxml_t *parser, const char *buffer, unsigned bufferlen, sxmltok_t* tokens, unsigned num_tokens);

/*
 you provide the sxml_parse() with a buffer of XML text for parsing
 the parser will successfully parse text data encoded in ascii, latin-1 or utf-8.
 it may also work with other encodings that are acsii extensions.
 
 sxml_parse is reentrant.
 in the case of return code SXML_ERROR_BUFFERDRY or SXML_ERROR_TOKENSFULL you are expected to call the function again after resolving the problem to continue parsing
 the following parser object stores all data required for SXML to continue from where it left of
*/

struct sxml_t
{
	unsigned bufferpos;	/* current offset into buffer - xml data before this position has been successfully parsed */
	unsigned ntokens;	/* number of tokens filled with valid data by parser */
	unsigned taglevel;	/* used internally - helps detect start and end of xml document */
};

void sxml_init(sxml_t *parser);

/*
 before you call sxml_parse() for the first time you have to initialize the parser object
 you may easily do that with the provided function sxml_init()

 on return 'ntokens' tells you how many output tokens have been filled with data
 depending on how you resolve SXML_ERROR_BUFFERDRY or SXML_ERROR_TOKENSFULL you may need to modifiy 'bufferpos' and 'ntokens' to correctly reflect the new buffer and tokens you provide
*/

/*
 unlike most XML parsers it does not use SAX callbacks or allocate a DOM tree
 instead you interpret the XML structure through a table of tokens

 a token can describe any of the following types
*/

typedef enum
{
	SXML_STARTTAG,	/* start tag describes the opening of an XML element */
	SXML_ENDTAG,	/* end tag is the closing of an XML element */

	SXML_CHARACTER,		/* character data can be escaped - you might want to unescape the string for correct interpretation */
	SXML_CDATA,			/* character data should be read as is - it is not escaped */

	/* and some other token types you might be interested in */
	SXML_INSTRUCTION,	/* can be used to identity the text encoding */
	SXML_DOCTYPE,		/* if you'd like to interpret DTD data */
	SXML_COMMENT		/* most likely you don't care about comments - but this is where you'll find them */
} sxmltype_t;

/*
 if you are familiar with the structure of an XML document most of these type names should sound familiar
 
 a token also has the following information
*/

struct sxmltok_t
{
	unsigned short type;	/* a token is one of the above sxmltype_t */
	unsigned short size;	/* the following number of tokens contain additional data related to this token - used for describing attributes */

	/* startpos and endpos together define a range within the provided text buffer - use these offsets with the buffer to extract the text value of the token */
	unsigned startpos;
	unsigned endpos;
};

/*
 let's walk though how to correctly interpret a token of type SXML_STARTTAG
 
 the element name can be extracted from the text buffer using startpos and endpos

 the attributes of the XML element are described in the following 'size' tokens
 an attribute will be described using 2 tokens - first token will be the attribute name, the second will be the value
 there is no specific type for attribute tokens. instead the types SXML_CDATA and SXML_CHARACTER are used to hint that attribute values might need to be unescaped

 when processing the tokens do not forget about 'size' - for token types you want to skip remember to also skip the additional token data
*/

#ifdef __cplusplus
}
#endif

#endif /* _SXML_H_INCLUDED */
