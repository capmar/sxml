#include "sxml.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned UINT;

/*
 MARK: Pretty print XML
 Example of simple processing of parsed token output.
*/
static void print_indent (UINT indentlevel)
{
	if (0 < indentlevel)
	{
		char fmt[8];
		sprintf (fmt, "%%%ds", indentlevel * 3);
		printf (fmt, " ");
	}
}

static void print_tokenvalue (const char* buffer, const sxmltok_t* token)
{
	char fmt[8];
	sprintf (fmt, "%%.%ds", token->endpos - token->startpos);
	printf (fmt, buffer + token->startpos);
}

static UINT print_chartokens (const char* buffer, const sxmltok_t tokens[], UINT num_tokens)
{
	UINT i;

	for (i= 0; i < num_tokens; i++)
	{
		const char* ampr;

		const sxmltok_t* token= tokens + i;
		if (token->type != SXML_CHARACTER)
			return i;

		ampr= buffer + token->startpos;
		assert (0 < token->endpos - token->startpos);

		if (*ampr != '&')
		{
			print_tokenvalue (buffer, token);
			continue;
		}

		switch (ampr[1])
		{
		case 'a':	printf ((ampr[2] == 'm') ? "&" : "'");	break;
		case 'g':	printf (">");	break;
		case 'l':	printf ("<");	break;
		case 'q':	printf ("\"");	break;
		default:
			assert (0);
			break;
		}
	}

	return num_tokens;
}

static void print_prettyxml (const char* buffer, const sxmltok_t tokens[], UINT num_tokens, UINT* indentlevel)
{
	UINT i;
	for (i= 0; i < num_tokens; i++)
	{
		const sxmltok_t* token= tokens + i;
		switch (token->type)
		{
			case SXML_STARTTAG:
			{
				UINT j;

				print_indent ((*indentlevel)++);
				printf ("<");
				print_tokenvalue (buffer, token);

				/* elem attributes are listed in the following tokens */
				for (j= 0; j < token->size; j++)
				{
					printf (" ");
					print_tokenvalue (buffer, &token[j + 1]);
					printf ("='");
					j+= print_chartokens (buffer, &token[j + 2], token->size - (j + 1));
					printf ("'");
				}

				puts (">");
				break;
			}

			case SXML_ENDTAG:
				print_indent (--(*indentlevel));
				printf ("</");
				print_tokenvalue (buffer, token);
				puts (">");
				break;


			/* Other token types you might be interested in: */
			/*
			case SXML_INSTRUCTION
			case SXML_DOCTYPE:
			case SXML_COMMENT:
			case SXML_CDATA:
			case SXML_CHARACTER:
			*/

			default:
				break;
		}

		/*
		 Tokens may contain additional data. Skip 'size' tokens to get the next token to proccess.
		 (see SXML_STARTTAG case above as an example of how attributes are specified)
		*/
		i+= token->size;
	}
}

/*
 MARK: Utility functions
 Useful for error reporting.
*/
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


/*
 MARK: main
 Minimal example showing how you may use SXML within a constrained environment with a fixed size input and output buffer.
*/

#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define COUNT(arr)	(sizeof (arr) / sizeof ((arr)[0]))

#define BUFFER_MAXLEN	1024


int main (int argc, const char* argv[])
{
	/* Input XML text */
	char buffer[BUFFER_MAXLEN];
	UINT bufferlen= 0;

	/* Output token table */
	sxmltok_t tokens[128];

	/* Used in example for pretty printing and error reporting */
	UINT indent= 0, lineno= 1;

	const char* path;
	FILE* file;

	/* Parser object stores all data required for SXML to be reentrant */
	sxml_t parser;
	sxml_init (&parser);

	/* Usage: sxml_test.exe test.xml */
	assert (argc == 2);
	path= argv[1];
	file= fopen (path, "rb");
	assert (file != NULL);
	
	for (;;)
	{
		sxmlerr_t err= sxml_parse (&parser, buffer, bufferlen, tokens, COUNT (tokens));
		if (err == SXML_SUCCESS)
			break;

		switch (err)
		{
			case SXML_ERROR_TOKENSFULL:
			{
				/*
				 Need to give parser more space for tokens to continue parsing.
				 We choose here to reuse the existing token table once tokens have been processed.

				 Example of some processing of the token data.
				 Instead you might be interested in creating your own DOM structure
				 or other processing of XML data useful to your application.
				*/
				print_prettyxml (buffer, tokens, parser.ntokens, &indent);

				/* Parser can now safely reuse all of the token table */
				parser.ntokens= 0;
				break;
			}

			case SXML_ERROR_BUFFERDRY:
			{
				/* 
				 Parser expects more XML data to continue parsing.
				 We choose here to reuse the existing buffer array.
				*/
				size_t len;

				/* Need to processs existing tokens before buffer is overwritten with new data */
				print_prettyxml (buffer, tokens, parser.ntokens, &indent);
				parser.ntokens= 0;

				/* For error reporting */
				lineno+= count_lines(buffer, parser.bufferpos);

				/*
				 Example of how to reuse buffer array.
				 Move unprocessed buffer content to start of array
				*/
				bufferlen-= parser.bufferpos;
				memmove (buffer, buffer + parser.bufferpos, bufferlen);

				/* 
				 If your buffer is smaller than the size required to complete a token the parser will endlessly call SXML_ERROR_BUFFERDRY.
				 You will most likely encounter this problem if you have XML comments longer than BUFFER_MAXLEN in size.
				 SXML_CHARACTER solves this problem by dividing the data over multiple tokens, but other token types remain affected.
				*/
				assert (bufferlen < BUFFER_MAXLEN);

				/* Fill remaining buffer with new data from file */
				len= fread (buffer + bufferlen, 1, BUFFER_MAXLEN - bufferlen, file);
				assert (0 < len);
				bufferlen+= len;

				/* Parser will now have to read from beginning of buffer to contiue */
				parser.bufferpos= 0;
				break;
			}

			case SXML_ERROR_XMLINVALID:
			{
				char fmt[8];

				/* Example of some simple error reporting */
				lineno+= count_lines (buffer, parser.bufferpos);
				fprintf(stderr, "Error while parsing line %d:\n", lineno);

				/* Print out contents of line containing the error */
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

	/* Sucessfully parsed XML file - flush remainig token output */
	print_prettyxml (buffer, tokens, parser.ntokens, &indent);
	return 0;
}
