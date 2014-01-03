#include "sxml.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef unsigned UINT;
typedef int BOOL;
#define FALSE	0
#define TRUE	(!FALSE)

//---

//MARK: string
//string functions work within memory range specified (excluding end)
//returns end if value not found

#define ISALPHA(c)   (isalpha(((unsigned char)(c))))
#define ISSPACE(c)   (isspace(((unsigned char)(c))))

static const char* str_findchr (const char* start, const char* end, int c)
{
	const char* it;

	assert (start <= end);
	assert (0 <= c && c <= 127);	//CHAR_MAX
	//memchr implementation will only work when searching for ascii characters within a utf8 string
	
	it= (const char*) memchr (start, c, end - start);
	return (it != NULL) ? it : end;
}

static const char* str_findstr (const char* start, const char* end, const char* needle)
{
	size_t needlelen;
	int first;
	assert  (start <= end);
	
	needlelen= strlen (needle);
	assert (0 < needlelen);
	first = (unsigned char) needle[0];

	while (start + needlelen <= end)
	{
		const char* it= (const char*) memchr (start, first, (end - start) - (needlelen - 1));
		if (it == NULL)
			break;

		if (memcmp (it, needle, needlelen) == 0)
			return it;

		start= it + 1;
	}

	return end;
}

static BOOL str_startswith (const char* start, const char* end, const char* prefix)
{
	ptrdiff_t nbytes;
	assert (start <= end);
	
	nbytes= strlen (prefix);
	if (end - start < nbytes)
		return FALSE;
	
	return memcmp (prefix, start, nbytes) == 0;
}

static BOOL str_endswith (const char* start, const char* end, const char* suffix)
{
	ptrdiff_t nbytes;
	assert (start <= end);
	
	nbytes= strlen (suffix);
	if (end - start < nbytes)
		return FALSE;
	
	return memcmp (suffix, end - nbytes, nbytes) == 0;
}

static const char* str_lspace (const char* start, const char* end)
{
	const char* it;	
	assert (start <= end);

	for (it= start; it != end && !ISSPACE (*it); it++)
		;

	return it;
}

//left trim whitespace
static const char* str_ltrim (const char* start, const char* end)
{
	const char* it;
	assert (start <= end);

	for (it= start; it != end && ISSPACE (*it); it++)
		;

	return it;
}

//right trim whitespace
static const char* str_rtrim (const char* start, const char* end)
{
	const char* it, *prev;
	assert (start <= end);

	for (it= end; start != it; it= prev)
	{
		prev= it - 1;
		if (!ISSPACE (*prev))
			return it;
	}
	
	return start;
}

//MARK: context

typedef struct
{
	const char* xml;
	UINT xmllen;
	sxmltok_t* tokens;
	UINT num_tokens;
} sxml_args_t;

#define args_getpos(args,state)	((args)->xml + (state)->bufferpos)
#define args_getend(args) ((args)->xml + (in)->xmllen)

//---

static BOOL state_pushtoken (sxml_parser* state, sxml_args_t* in, sxmltype_t type, const char* start, const char* end)
{
	sxmltok_t* token;
	UINT i= state->ntokens++;
	if (in->num_tokens < state->ntokens)
		return FALSE;
	
	token= &in->tokens[i];
	token->type= type;
	token->startpos= start - in->xml;
	token->endpos= end - in->xml;
	token->size= 0;

	switch (type)
	{
		case SXML_STARTTAG:	state->taglevel++;	break;

		case SXML_ENDTAG:
			assert (0 < state->taglevel);
			state->taglevel--;
			break;

		default:
			break;
	}

	return TRUE;
}

static sxmlerr_t state_commit (sxml_parser* state, const sxml_args_t* in, const char* pos)
{
	if (in->num_tokens < state->ntokens)
		return SXML_ERROR_TOKENSFULL;
		
	state->bufferpos= pos - in->xml;
	return SXML_SUCCESS;
}

static sxmltok_t* state_lasttoken (const sxml_parser* state, const sxml_args_t* in)
{
	sxmltok_t *tokens= in->tokens;
	return (0 < state->ntokens) ? tokens + (state->ntokens - 1) : NULL;
}


//MARK: parse

#define TAG_LEN(str)	(sizeof (str) - 1)
#define TAG_MINSIZE	3

static sxmlerr_t parse_attributes (sxml_parser* state, sxml_args_t* in, const char* start, const char* end)
{
	const char* name;
	sxmltok_t* token= state_lasttoken (state, in);
	assert (token != NULL);

	name= str_ltrim (start, end);
	while (name < end)
	{
		//name
		const char* space, *quot, *value;
		const char* eq= str_findchr (name, end, '=');
		if (eq == end)
			return SXML_ERROR_XMLINVALID;

		space= str_rtrim (name, eq);
		state_pushtoken (state, in, SXML_CDATA, name, space);

		//value
		quot= str_ltrim (eq + 1, end);
		if (quot == end || !(*quot == '\'' || *quot == '"'))
			return SXML_ERROR_XMLINVALID;

		value= quot + 1;
		quot= str_findchr (value, end, *quot);
		if (quot == end)
			return SXML_ERROR_XMLINVALID;

		state_pushtoken (state, in, SXML_CHARACTER, value, quot);

		//---
		token->size+= 2;
		name= str_ltrim (quot + 1, end);
	}

	return SXML_SUCCESS;
}

static sxmlerr_t parse_comment (sxml_parser* state, sxml_args_t* in)
{
	static const char STARTTAG[]= "<!--";
	static const char ENDTAG[]= "-->";

	const char* dash;
	const char* start= args_getpos (in, state);
	const char* end= args_getend (in);
	if (end - start < TAG_LEN (STARTTAG))
		return SXML_ERROR_BUFFERDRY;

	if (!str_startswith (start, end, STARTTAG))
		return SXML_ERROR_XMLINVALID;

	start+= TAG_LEN (STARTTAG);
	dash= str_findstr (start, end, ENDTAG);
	if (dash == end)
		return SXML_ERROR_BUFFERDRY;

	state_pushtoken (state, in, SXML_COMMENT, start, dash);
	return state_commit (state, in, dash + TAG_LEN (ENDTAG));
}

static sxmlerr_t parse_instruction (sxml_parser* state, sxml_args_t* in)
{
	static const char STARTTAG[]= "<?";
	static const char ENDTAG[]= "?>";

	const char* quest;
	const char* start= args_getpos (in, state);
	const char* end= args_getend (in);
	assert (TAG_MINSIZE <= end - start);

	if (!str_startswith (start, end, STARTTAG))
		return SXML_ERROR_XMLINVALID;

	start+= TAG_LEN (STARTTAG);
	quest= str_findstr (start, end, ENDTAG);
	if (quest == end)
		return SXML_ERROR_BUFFERDRY;

	state_pushtoken (state, in, SXML_INSTRUCTION, start, quest);
	parse_attributes (state, in, start, quest);
	return state_commit (state, in, quest + TAG_LEN (ENDTAG));
}

static sxmlerr_t parse_doctype (sxml_parser* state, sxml_args_t* in)
{
	static const char STARTTAG[]= "<!DOCTYPE";
	static const char ENDTAG[]= "]>";

	const char* bracket;
	const char* start= args_getpos (in, state);
	const char* end= args_getend (in);
	if (end - start < TAG_LEN (STARTTAG))
		return SXML_ERROR_BUFFERDRY;

	if (!str_startswith (start, end, STARTTAG))
		return SXML_ERROR_BUFFERDRY;

	start+= TAG_LEN (STARTTAG);
	bracket= str_findstr (start, end, ENDTAG);
	if (bracket == end)
		return SXML_ERROR_BUFFERDRY;

	state_pushtoken (state, in, SXML_DOCTYPE, start, bracket);
	return state_commit (state, in, bracket + TAG_LEN (ENDTAG));
}

static sxmlerr_t parse_start (sxml_parser* state, sxml_args_t* in)
{	
	BOOL empty;
	sxmlerr_t err;
	const char* gt, *name, *space;
	const char* start= args_getpos (in, state);
	const char* end= args_getend (in);
	assert (TAG_MINSIZE <= end - start);

	if (!(start[0] == '<' && ISALPHA (start[1])))
		return SXML_ERROR_XMLINVALID;

	start++;		
	gt= str_findchr (start, end, '>');
	if (gt == end)
		return SXML_ERROR_BUFFERDRY;

	empty= str_endswith (start, gt + 1, "/>");
	end= (empty) ? gt - 1 : gt;

	//---

	name= start;
	space= str_lspace (name, end);
	if (!state_pushtoken (state, in, SXML_STARTTAG, name, space))
		return SXML_ERROR_TOKENSFULL;

	err= parse_attributes (state, in, space, end);
	if (err != SXML_SUCCESS)
		return err;

	//---

	if (empty)
		state_pushtoken (state, in, SXML_ENDTAG, name, space);

	return state_commit (state, in, gt + 1);
}

static sxmlerr_t parse_end (sxml_parser* state, sxml_args_t* in)
{
	const char* gt, *space;
	const char* start= args_getpos (in, state);
	const char* end= args_getend (in);
	assert (TAG_MINSIZE <= end - start);

	if (!(str_startswith (start, end, "</") && ISALPHA (start[2])))
		return SXML_ERROR_XMLINVALID;

	start+= 2;	
	gt= str_findchr (start, end, '>');
	if (gt == end)
		return SXML_ERROR_BUFFERDRY;

	//test for no characters beyond elem name
	space= str_lspace (start, gt);
	if (str_ltrim (space, gt) != gt)
		return SXML_ERROR_XMLINVALID;

	state_pushtoken (state, in, SXML_ENDTAG, start, space);
	return state_commit (state, in, gt + 1);
}

static sxmlerr_t parse_cdata (sxml_parser* state, sxml_args_t* in)
{
	static const char STARTTAG[]= "<![CDATA[";
	static const char ENDTAG[]= "]]>";

	const char* bracket;
	const char* start= args_getpos (in, state);
	const char* end= args_getend (in);
	if (end - start < TAG_LEN (STARTTAG))
		return SXML_ERROR_BUFFERDRY;

	if (!str_startswith (start, end, STARTTAG))
		return SXML_ERROR_XMLINVALID;

	start+= TAG_LEN (STARTTAG);
	bracket= str_findstr (start, end, ENDTAG);
	if (bracket == end)
		return SXML_ERROR_BUFFERDRY;

	state_pushtoken (state, in, SXML_CDATA, start, bracket);
	return state_commit (state, in, bracket + TAG_LEN (ENDTAG));
}

static sxmlerr_t parse_characters (sxml_parser* state, sxml_args_t* in)
{
	const char* start= args_getpos (in, state);
	const char* end= args_getend (in);

	const char* lt= str_findchr (start, end, '<');
	if (lt == end)
		return SXML_ERROR_BUFFERDRY;

	if (lt != start)
		state_pushtoken (state, in, SXML_CHARACTER, start, lt);

	return state_commit (state, in, lt);
}

//MARK: sxml
//Public API inspired by the JSON parser jsmn
//http://zserge.com/jsmn.html

void sxml_init (sxml_parser *parser)
{
    parser->bufferpos= 0;
    parser->ntokens= 0;
	parser->taglevel= 0;
}

#define ROOT_FOUND(state)	(0 < (state)->taglevel)
#define ROOT_PARSED(state)	((state)->taglevel == 0)

sxmlerr_t sxml_parse(sxml_parser *parser, const char *xml, UINT xmllen, sxmltok_t tokens[], UINT num_tokens)
{
	sxml_parser temp= *parser;
	const char* end= xml + xmllen;
	
	sxml_args_t in;
	in.xml= xml;
	in.xmllen= xmllen;
	in.tokens= tokens;
	in.num_tokens= num_tokens;

	//---

	while (!ROOT_FOUND (&temp))
	{
		sxmlerr_t err;
		const char* start= args_getpos (&in, &temp);
		const char* lt= str_ltrim (start, end);
		if (end - lt < TAG_MINSIZE)
			return SXML_ERROR_BUFFERDRY;

		if (*lt != '<')
			return SXML_ERROR_XMLINVALID;

		temp.bufferpos= lt - xml;
		*parser= temp;	//commit

		//---

		switch (lt[1])
		{
		case '?':	err= parse_instruction (&temp, &in);	break;
		case '!':	err= parse_doctype (&temp, &in);	break;
		default:	err= parse_start (&temp, &in);	break;
		}

		if (err != SXML_SUCCESS)
			return err;

		*parser= temp;	//commit
	}

	//---

	while (!ROOT_PARSED (&temp))
	{
		const char* lt;
		sxmlerr_t err= parse_characters (&temp, &in);
		if (err != SXML_SUCCESS)
			return err;

		*parser= temp;	//commit

		//---

		lt= args_getpos (&in, &temp);
		assert (*lt == '<');		
		if (end - lt < TAG_MINSIZE)
			return SXML_ERROR_BUFFERDRY;

		switch (lt[1])
		{
		case '?':	err= parse_instruction (&temp, &in);		break;
		case '/':	err= parse_end (&temp, &in);	break;
		case '!':	err= (lt[2] == '-') ? parse_comment (&temp, &in) : parse_cdata (&temp, &in);	break;
		default:	err= parse_start (&temp, &in);	break;
		}

		if (err != SXML_SUCCESS)
			return err;

		*parser= temp;	//commit
	}

	return SXML_SUCCESS;
}