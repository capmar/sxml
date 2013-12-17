#include "sxml.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned UINT;
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define COUNT(arr)	(sizeof (arr) / sizeof ((arr)[0]))

#define BUFFER_MAXLEN	1024

//MARK: refill_buffer

static UINT count_lines (const char* buffer, UINT bufferlen)
{
	const char* end= buffer + bufferlen;

	for (UINT i= 0; ; i++)
	{
		buffer= (const char*) memchr (buffer, '\n', end - buffer);
		if (buffer == NULL)
			return i;

		buffer++;
	}
}

static UINT refill_buffer (char* buffer, UINT* bufferlen, FILE* file, UINT pos)
{
	assert (pos <= *bufferlen);
	assert (*bufferlen <= BUFFER_MAXLEN);

	UINT lineno= count_lines (buffer, pos);

	*bufferlen-= pos;
	memmove (buffer, buffer + pos, *bufferlen);
	
	*bufferlen+= fread (buffer + *bufferlen, 1, BUFFER_MAXLEN - *bufferlen, file);
	return lineno;
}

//MARK: flush_tokens

static void print_indentspaces (UINT nspaces)
{
	if (0 < nspaces)
	{
		char fmt[8];
		sprintf (fmt, "%%%ds", nspaces);
		printf (fmt, " ");
	}
}

static void print_tokenstring (const sxmltok_t* token, const char* buffer)
{
	char fmt[8];
	sprintf (fmt, "%%.%ds", token->end - token->start);
	printf (fmt, buffer + token->start);
}

static void flush_tokens (const char* buffer, const sxmltok_t tokens[], UINT ntokens, UINT* indent)
{
	for (UINT i= 0; i < ntokens; i++)
	{
		const sxmltok_t* token= tokens + i;
		switch (token->type)
		{
		case SXML_STARTTAG:
			{
			print_indentspaces ((*indent)++ * 3);
			printf ("<");
			print_tokenstring (token, buffer);
			for (UINT j= 0; j < token->size; j++)
			{
				printf (" ");
				print_tokenstring (token + (j * 2 + 1), buffer);
				printf ("='");
				print_tokenstring (token + (j * 2 + 2), buffer);
				printf ("'");
			}

			puts (">");
			break;
			}

		case SXML_ENDTAG:
			print_indentspaces (--(*indent) * 3);
			printf ("</");
			print_tokenstring (token, buffer);
			puts (">");
			break;

		//case SXML_INSTRUCTION:
		//case SXML_DOCTYPE:
		//case SXML_COMMENT:
		//case SXML_CDATA:
		//case SXML_CHARACTER:
		default:
			break;
		}

		i+= token->size;
	}
}

//MARK: report_error

static void report_error (const char* buffer, int bufferlen, UINT pos, UINT lineno)
{
	lineno+= count_lines (buffer, pos);
	fprintf(stderr, "Error while parsing line %d:\n", lineno);

	char fmt[16];
	sprintf (fmt, "%%.%ds", MIN (bufferlen - pos, 72));
	fprintf (stderr, fmt, buffer + pos);
}

//MARK: main

int main (int argc, const char* argv[])
{
	sxmltok_t tokens[128];
	char buffer[BUFFER_MAXLEN];
	
	UINT bufferlen= 0;
	UINT lineno= 1, indent= 0;

	sxml_parser parser;
	sxml_init (&parser);

	assert (argc == 2);
	const char* path= argv[1];
	FILE* file= fopen (path, "rb");
	
	for (;;)
	{
		sxmlerr_t err= sxml_parse (&parser, buffer, bufferlen, tokens, COUNT (tokens));
		if (err == SXML_SUCCESS)
			break;

		switch (err)
		{
		case SXML_ERROR_PART:
			flush_tokens (buffer, tokens, parser.toknext, &indent);
			parser.toknext= 0;

			lineno+= refill_buffer (buffer, &bufferlen, file, parser.pos);
			parser.pos= 0;
			break;
		
		case SXML_ERROR_NOMEM:
			flush_tokens (buffer, tokens, parser.toknext, &indent);
			parser.toknext= 0;
			break;
		
		case SXML_ERROR_INVAL:
			report_error (buffer, bufferlen, parser.pos, lineno);
			abort();
			break;

		default:
			assert (0);
			break;
		}
	}

	fclose (file);
	flush_tokens (buffer, tokens, parser.toknext, &indent);
}
