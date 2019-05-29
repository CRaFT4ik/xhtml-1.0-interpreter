
/* Coded by Eldar Timraleev (aka CRaFT4ik) © 2019 */

%{
	#define alloca malloc

	#include <stdio.h>
	#include <stdlib.h>
	#include <ctype.h>
	#include <string.h>
	#include <stdarg.h>
	#include <locale.h>

	extern int yylineno; // Номер строки с ошибкой.;
	extern int DEBUG_MODE;

	void yyerror(char *str, ...)
	{
		stack_show();

		if (str == NULL) return;
		int need_extra_out = 0;
		int i, *p = &str; char c, *s;

		printf("\nERROR: ");

		while (*str != '\0')
		{
			if (*str == '%' && (*(str + 1) == 'd' || *(str + 1) == 'c' || *(str + 1) == 's'))
			{
				str++;
				switch (*str)
				{
					case 'd':
						p += 1;
						i = *(int *) p;
						printf("%d", i);
						break;
					case 'c':
						p += 1;
						c = *(int *) p;
						printf("%c", c);
						break;
					case 's':
						p += 1;
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
			printf("\nyyparse(): parsing successful! :)\n\n");
		else
			printf("\nyyparse(): error during parsing! :(\n\n");

		stack_free();
		attr_free();
		tags_free();

		system("pause");
		return 0;

usage:  printf("usage: %s <params> <input file>\nparams: -d - use debug mode\n", argv[0]);
		system("pause");
		return 0;
	}
