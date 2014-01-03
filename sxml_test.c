#include "sxml.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned UINT;

//---

//MARK: pretty print xml
//example of simple processing of token data

static void print_indent (UINT indentlevel)
{
	if (0 < indentlevel)
	{
		char fmt[8];
		sprintf (fmt, "%%%ds", indentlevel * 3);
		printf (fmt, " ");
	}
}

static void print_tokenname (const sxmltok_t* token, const char* buffer)
{
	char fmt[8];
	sprintf (fmt, "%%.%ds", token->endpos - token->startpos);
	printf (fmt, buffer + token->startpos);
}

static void print_prettyxml (const char* buffer, const sxmltok_t tokens[], UINT ntokens, UINT* indentlevel)
{
	UINT i;
	for (i= 0; i < ntokens; i++)
	{
		const sxmltok_t* token= tokens + i;
		switch (token->type)
		{
			case SXML_STARTTAG:
			{
				UINT j;

				print_indent ((*indentlevel)++);
				printf ("<");
				print_tokenname (token, buffer);

				//elem attributes are listed in the following tokens
				for (j= 0; j < token->size; j+= 2)
				{
					printf (" ");
					print_tokenname (&token[j + 1], buffer);
					printf ("='");
					print_tokenname (&token[j + 2], buffer);
					printf ("'");
				}

				puts (">");
				break;
			}

			case SXML_ENDTAG:
				print_indent (--(*indentlevel));
				printf ("</");
				print_tokenname (token, buffer);
				puts (">");
				break;

			//other token types you might be interested in
			/*
			case SXML_INSTRUCTION:
			case SXML_DOCTYPE:
			case SXML_COMMENT:
			case SXML_CDATA:
			case SXML_CHARACTER:
			*/

			default:
				break;
		}

		i+= token->size;
	}
}

//MARK: utility functions
//useful for error reporting
static UINT count_lines (const char* buffer, UINT bufferlen)
{
	const char* end= buffer + bufferlen;
	const char* it= buffer;
	UINT i;

	for (i= 0; ; i++)
	{
		it= (const char*) memchr (it, '\n', end - it);
		if (it == NULL)
			return i;

		it++;
	}
}


//MARK: main
//minimal example showing how you may use SXML with a fixed size input and output buffer

#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define COUNT(arr)	(sizeof (arr) / sizeof ((arr)[0]))

#define BUFFER_MAXLEN	1024


int main (int argc, const char* argv[])
{
	//input
	char buffer[BUFFER_MAXLEN];
	UINT bufferlen= 0;

	//output
	sxmltok_t tokens[128];

	//used in example for pretty printing and error reporting
	UINT indent= 0, lineno= 1;

	const char* path;
	FILE* file;

	//parser object stores all data required for SXML to be reentrant
	sxml_parser parser;
	sxml_init (&parser);

	//usage: sxml_test.exe test.xml
	assert (argc == 2);
	path= argv[1];
	file= fopen (path, "rb");
	
	for (;;)
	{
		sxmlerr_t err= sxml_parse (&parser, buffer, bufferlen, tokens, COUNT (tokens));
		if (err == SXML_SUCCESS)
			break;

		switch (err)
		{
			case SXML_ERROR_TOKENSFULL:
			{
				//need to give parser more space for tokens to continue parsing
				//we choose here to reuse the existing token table once tokens have been processed

				//- example of some processing of the token data
				//instead you might be interested in creating your own DOM structure
				//or other processing of XML data useful for application
				print_prettyxml (buffer, tokens, parser.ntokens, &indent);

				parser.ntokens= 0;
				break;
			}

			case SXML_ERROR_BUFFERDRY:
			{
				//parser expects more xml data to continue parsing
				//we choose here to reuse the existing buffer array
				size_t len;

				//need to processs existing tokens before buffer is overwritten with new data
				print_prettyxml (buffer, tokens, parser.ntokens, &indent);
				parser.ntokens= 0;

				//for error reporting
				lineno+= count_lines(buffer, parser.bufferpos);

				//- example of how to reuse buffer array
				//move unprocessed buffer content to start of array
				bufferlen-= parser.bufferpos;
				memmove (buffer, buffer + parser.bufferpos, bufferlen);

				//fill remaining buffer with new data from file
				len= fread (buffer + bufferlen, 1, BUFFER_MAXLEN - bufferlen, file);
				assert (0 < len);
				bufferlen+= len;

				parser.bufferpos= 0;
				break;
			}

			case SXML_ERROR_XMLINVALID:
			{
				char fmt[16];

				//example of some simple error reporting
				lineno+= count_lines (buffer, parser.bufferpos);
				fprintf(stderr, "Error while parsing line %d:\n", lineno);

				//print out contents of line containing the error
				sprintf (fmt, "%%.%ds", MIN (bufferlen - parser.bufferpos, 72));
				fprintf (stderr, fmt, buffer + parser.bufferpos);

				abort();
				break;
			}

		default:
			assert (0);
			break;
		}
	}

	fclose (file);

	//sucessfully parsed xml file
	//flush remainig token output
	print_prettyxml (buffer, tokens, parser.ntokens, &indent);
}
