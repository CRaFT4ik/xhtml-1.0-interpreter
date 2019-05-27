/*
	http://westes.github.io/flex/manual/

	+ - 1 или более совпадений подряд
	* - 0 или более совпадений подряд
	? - 0 или 1 совпадение
*/

%{

#include "y_tab.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "tags.c"
#include "stack.c"

#define DEBUG

void debug(char *str)
{
	#ifdef DEBUG
	printf("%s", str);
	#endif
}

%}

%option stack yylineno noyywrap

%x state_start_tag_need_closed
%s state_expected_end_tag
%x state_doctype_expected

EC					([ \t\r\n]+)
Digit				[0-9]
Letter				[a-zA-Z]
Title				({Letter}|[_:])({Letter}|{Digit}|[-._:])*
AnyText				[^<&]*

XMLVersionNum		"1.0"
XMLVersionInfo		{EC}"version"{EC}?"="{EC}?(\"{XMLVersionNum}\"|"'"{XMLVersionNum}"'")
XMLEncodingName		{Letter}("-"|{Letter}|{Digit}|[._])*
XMLEncodingInfo		{EC}"encoding"{EC}?"="{EC}?("'"{XMLEncodingName}"'"|\"{XMLEncodingName}\")
XMLSAInfo			{EC}"standalone"{EC}?"="{EC}?((\"("yes"|"no")\")|("'"("yes"|"no")"'"))
XMLInfo				"<?"[xX][mM][lL]{XMLVersionInfo}{XMLEncodingInfo}?{XMLSAInfo}?{EC}?"?>"
UnknownInfo     	"<?"{Title}[^?]*[?]+([^?>][^?]*[?]+)*">"
PIInfo          	"<?"{Title}({EC}[^?]*)?[?]+([^?>][^?]*[?]+)*">"

DoctypeDecl			"<!"[dD][oO][cC][tT][yY][pP][eE][^<!>]*">"
DoctypeInfo			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"

Comment         	"<!--"[^-]*[-]{2,}([^->][^-]*[-]{2,})*">"

Tags_1				"html"|"head"|"body"
StartTag			{Tags_1}

%%

{Comment}       	{
						debug("COMMENT");
                    	return COMMENT;
                	}

{XMLInfo}			{
						debug("XML_INFO");
						return XML_INFO;
					}

{PIInfo}			{
						debug("PI_INFO");
						return PI_INFO;
					}

{DoctypeDecl}		{
						yy_push_state(state_doctype_expected);
						yyless(0);
					}

<state_doctype_expected>{
{DoctypeInfo}		{
						debug("DOCTYPE_INFO");
						yy_pop_state();
						return DOCTYPE_INFO;
					}

{DoctypeDecl}		{
						yyerror("wrong DOCTYPE declaration format");
						yy_pop_state();
						return LEX_ERROR;
					}

[ \t\r\n]			;
}

"<"{StartTag}		{
						debug("TAG_START");
						stack_push(yytext + 1);
						yy_push_state(state_start_tag_need_closed);
						return TAG_START;
					}

"<"{Title}			{
						debug("LEX_ERROR");
						yyerror("unknown tag %s>", yytext);
						return LEX_ERROR;
					}

<state_start_tag_need_closed>{
"/>"				{
						debug("TAG_END_EMPTY");
						stack_pop(yytext, 1);
						yy_pop_state();
						return TAG_END_EMPTY;
					}

[>]					{
						yy_pop_state();
						yy_push_state(state_expected_end_tag);
						return yytext[0];
					}

[ \t\r\n]			debug(yytext);
}

<state_expected_end_tag>{
"</"{StartTag}">"	{
						debug("TAG_END");
						stack_pop(yytext, 0);
						yy_pop_state();
						return TAG_END;
					}

"</"{Title}">"		{
						debug("LEX_ERROR");
						yyerror("unknown tag %s", yytext);
						yy_pop_state();
						return LEX_ERROR;
					}
}

[\r\n]+				debug(yytext);

%%