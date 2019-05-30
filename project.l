/*
	http://westes.github.io/flex/manual/
	https://www.w3.org/2010/04/xhtml10-strict.html

	+ - 1 или более совпадений подряд
	* - 0 или более совпадений подряд
	? - 0 или 1 совпадение

	TODO:
		[+] сделать проверку на пустоту тега (может ли иметь вид </tag> или <tag />);
		[-] сделать проверку для attributes REQUIRED rule;
		[-] сделать проверку на допустимые значения атрибутов;
		[-] нет очистки памяти для attributes.c.
*/

%{

#include "y.tab.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "tags.c"
#include "attributes.c"
#include "stack.c"

int DEBUG_MODE = 0;

void debug(char *str)
{
	if (DEBUG_MODE)
		printf("%s", str);
}

%}

%option stack yylineno noyywrap

%x state_start_tag_need_closed
%s state_expected_end_tag
%x state_doctype_expected
%x state_inside_cdata

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
DoctypeInfo			"<!DOCTYPE"{EC}+"html"{EC}+"PUBLIC"{EC}+"\"-//W3C//DTD XHTML 1.0 Strict//EN\""{EC}+("\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\""|"\"DTD/xhtml1-strict.dtd\""){EC}*">"

Comment         	"<!--"([^-]|-[^-]|--+[^->])*-*"-->"
CDataStart			"<![CDATA["
CDataEnd			"]]>"

AttrName			{Title}
AttrValue        	(\"[^<"]*\")|("'"[^<']*"'")
Attribute			{AttrName}{EC}?[=]{EC}?{AttrValue}

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

{CDataStart}		{
						yy_push_state(state_inside_cdata);
					}

<state_inside_cdata>{
{CDataEnd}			{
						debug("CDATA");
						yy_pop_state();
						return CDATA;
					}

[ \t\r\n]|.?		; // Пропускаем любые символы.
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
						yyerror("wrong DOCTYPE declaration format; use: %s", "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
						yy_pop_state();
						return LEX_ERROR;
					}

[ \t\r\n]			;
}

"<"{Title}			{
						debug("<TAG_START");
						stack_push(yytext + 1);
						yy_push_state(state_start_tag_need_closed);
						return TAG_START;
					}

<state_start_tag_need_closed>{
"/>"				{
						debug("/>");
						stack_pop(yytext, 1);
						yy_pop_state();
						return TAG_END_EMPTY;
					}

[>]					{
						debug(">");
						yy_pop_state();
						yy_push_state(state_expected_end_tag);
						return yytext[0];
					}

{Attribute}			{
						debug("ATTRIBUTE");
						stack_push_attribute(yytext);
						return ATTRIBUTE;
					}

[ \t\r\n]			{	debug(yytext);	}

[^<&>"']			{
						debug("LEX_ERROR");
						yyerror("unknown symbol '%s'", yytext);
						return LEX_ERROR;
					}
}

<state_expected_end_tag>{
"</"{Title}">"		{
						debug("</TAG_END>");

						char tg[32];
						sprintf(tg, "%s", yytext + 2);
						*(tg + strlen(tg) - 1) = '\0';

						stack_pop(tg, 0);
						yy_pop_state();
						return TAG_END;
					}
}

[\r\n]+				{	debug(yytext);	}
.?                	{	debug(yytext);	} // Любой не распознанный символ (обычно простой текст).

%%
