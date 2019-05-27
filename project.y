
/* Coded by Eldar Timraleev (aka CRaFT4ik) © 2019 */

%{
	#define alloca malloc

	#include <stdio.h>
	#include <stdlib.h>
	#include <ctype.h>
	#include <string.h>
	#include <stdarg.h>

	extern int yylineno; // Номер строки с ошибкой.

	/*
	void yyerror(char *str)
	{
		printf("\nERROR: %s :: (line #%d of the input file)\n", str, yylineno);
		system("pause");
		exit(-1);
	}*/

	void yyerror(char *str, ...)
	{
		if (str == NULL) return;
		int need_extra_out = 0;
		int i; char c, *s, *p = (char *) &str;

		printf("\nERROR: ");

		while (*str != '\0')
		{
			if (*str == '%' && *(str + 1) != '\0')
			{
				str++;
				switch (*str)
				{
					case 'd':
						p += 8;
						i = *(int *) p;
						printf("%d", i);
						break;
					case 'c':
						p += 8;
						c = *(int *) p;
						printf("%c", c);
						break;
					case 's':
						p += 8;
						s = *(char **) p;
						printf("%s", s);
						break;
				}
			} else
				printf("%c", *str);

			str++;
		}

		printf(" :: (line #%d of the input file)\n", yylineno);
		system("pause");
		exit(-1);
	}
%}

%start doc_page

%token XML_INFO PI_INFO DOCTYPE_INFO COMMENT
%token TAG_START TAG_ATTRIBUTE TAG_END_EMPTY TAG_END
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

tag_attribute	:	tag_attribute TAG_ATTRIBUTE
				|	{ yyval = 0; }
				;

tag_tail		:	TAG_END_EMPTY
				|	'>' content TAG_END
				;

content			:	content tag
				|	other
				;

%%

	int main(int argc, char *argv[])
	{
		extern FILE *yyin;

		if (argc != 2)
		{
			printf("usage: %s <input file>\n", argv[0]);
			system("pause");
			return;
		} else
			yyin = fopen(argv[1], "r");

		if (yyin == NULL)
		{
			yyerror("file was not opened");
			system("pause");
			return 0;
		}

		tags_init();
		stack_init();

		if (!yyparse())
			printf("\nyyparse(): parsing successful! :)\n\n");
		else
			printf("\nyyparse(): error during parsing! :(\n\n");

		stack_free();
		tags_free();

		system("pause");
		return 0;
	}
