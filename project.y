
/* Coded by Eldar Timraleev (aka CRaFT4ik) © 2019 */

%{
	#define alloca malloc

	#include <stdio.h>
	#include <stdlib.h>
	#include <ctype.h>
	#include <string.h>
	#include <stdarg.h>
	#include <locale.h>

	extern int yylineno; // Номер строки с ошибкой.
	extern int DEBUG_MODE;

	void yyerror(char *str, ...)
	{
		if (str == NULL) return;

		if (DEBUG_MODE)
			stack_show();

		printf("\nERROR: ");

		va_list ap;
		va_start(ap, str);
		while (*str != '\0')
		{
			if (*str == '%' && *(str + 1) != '\0')
			{
				str++;
				switch (*str)
				{
					case 'd':
						int i = va_arg(ap, int);
						printf("%d", i);
						break;
					case 'c':
						char c = va_arg(ap, char);
						printf("%c", c);
						break;
					case 's':
						char *s = va_arg(ap, char *);
						printf("%s", s);
						break;
					default:
						printf("yyerror: unknown parameter '%c'\n", *str);
						break;
				}
			} else
				printf("%c", *str);

			str++;
		}
		va_end(ap);

		printf(" :: (line #%d of the input file)\n", yylineno);
		printf("See cheat sheet here: %s\n", "https://www.w3.org/2010/04/xhtml10-strict.html");

		if (DEBUG_MODE) system("pause");
		exit(-1);
	}
%}

%start doc_page

%token XML_INFO PI_INFO DOCTYPE_INFO COMMENT CDATA
%token TAG_START ATTRIBUTE TAG_END_EMPTY TAG_END
%token LEX_ERROR

%%

doc_page		:	entry tag other
				;

entry			:	XML_INFO other DOCTYPE_INFO other
				|	other DOCTYPE_INFO other
				;

other			:	other COMMENT
				|	other PI_INFO
				|	{ yyval = 0; }
				;

tag				:	TAG_START tag_attribute tag_tail
				;

tag_attribute	:	tag_attribute ATTRIBUTE
				|	{ yyval = 0; }
				;

tag_tail		:	TAG_END_EMPTY
				|	'>' content TAG_END
				;

content			:	content tag other
				|	other
				|	CDATA
				;

%%

	int main(int argc, char *argv[])
	{
		setlocale(LC_ALL, "rus");
		extern FILE *yyin;

		if (argc == 1 || argc > 3)
			goto usage;
		else if (argc == 2)
		{
			if (strcmp(argv[1], "-h") == 0)
				goto usage;
			yyin = fopen(argv[1], "r");
		} else if (argc == 3)
		{
			if (strcmp(argv[1], "-d") == 0)
				DEBUG_MODE = 1;
			else
				goto usage;
			yyin = fopen(argv[2], "r");
		}

		if (yyin == NULL)
		{
			yyerror("file was not opened");
			system("pause");
			return 0;
		}

		tags_init();
		attr_init();
		stack_init();

		if (!yyparse())
			printf("\n\nyyparse(): parsing successful! :)\nCompliance with XHTML 1.0 Strict standard.\n\n");
		else
			printf("\n\nyyparse(): error during parsing! :(\n\n");

		stack_free();
		attr_free();
		tags_free();

		if (DEBUG_MODE) system("pause");
		return 0;

usage:  printf("usage: %s <params> <input file>\nparams: -d - use debug mode\n", argv[0]);
		if (DEBUG_MODE) system("pause");
		return 0;
	}
