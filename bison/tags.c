// https://www.w3.org/2010/04/xhtml10-strict.html

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <stdarg.h>
#include "y.tab.h"

#define NTAGS 100	// Макс. количество тегов.

typedef struct tag_st
{
	char name[32];
	int is_empty_tag;				 // Является ли тег пустым (закрывающая форма <tg />).

	struct tag_st *may_list[NTAGS];	 // Список тегов, которые может содержать \
									 // данный тег.
	struct tag_st *min_list[NTAGS];	 // Список тегов, которые должен содержать \
									 // данный тег (хотя бы один из этих).
	struct tag_st *must_list[NTAGS]; // Список тегов, которые должен содержать \
									 // данный тег (хотя бы один раз каждый).
} tag;

static tag all_tags[NTAGS];
static int all_tags_counter;

int tags_is_may_list_contains(tag *tg, char *sought_for)
{
	for (int i = 0; i < sizeof(tg->may_list) / sizeof(tg->may_list[0]); i++)
	{
		if (tg->may_list[i] == NULL) break;
		if (strcmp(tg->may_list[i]->name, sought_for) == 0)
			return 1;
	}

	return 0;
}

int tags_is_min_list_contains(tag *tg, char *sought_for)
{
	for (int i = 0; i < sizeof(tg->min_list) / sizeof(tg->min_list[0]); i++)
	{
		if (tg->min_list[i] == NULL) break;
		if (strcmp(tg->min_list[i]->name, sought_for) == 0)
			return 1;
	}

	return 0;
}

int tags_is_must_list_contains(tag *tg, char *sought_for)
{
	for (int i = 0; i < sizeof(tg->must_list) / sizeof(tg->must_list[0]); i++)
	{
		if (tg->must_list[i] == NULL) break;
		if (strcmp(tg->must_list[i]->name, sought_for) == 0)
			return 1;
	}

	return 0;
}

tag * tags_get(char *name)
{
	for (int i = 0; i < all_tags_counter; i++)
		if (strcmp(all_tags[i].name, name) == 0)
			return &all_tags[i];

	return NULL;
}

static tag * tags_alloc(char *name)
{
	if (all_tags_counter >= sizeof(all_tags) / sizeof(all_tags[0]))
		yyerror("tags_alloc: memory of array is full");

	tag *tg = &all_tags[all_tags_counter++];
	sprintf(tg->name, "%s", name);
	return tg;
}

// Создает тег, если его нет.
// Если тег существует, вернет указатель на существующий.
tag * tags_add_empty(char *name)
{
	tag *tmp_tg = tags_get(name), *tg;
	if (tmp_tg != NULL)
	{
		tmp_tg->is_empty_tag = 1;
		return tmp_tg;
	}

	tg = tags_alloc(name);
	memset(tg, 0, sizeof(tag));
	sprintf(tg->name, "%s", name);
	tg->is_empty_tag = 1;

	return tg;
}

// Заполняет список may_list. Если какого-либо тега не существовало,
// он будет создан. Последний параметр должен быть NULL.
tag * tags_add_may_list(char *name, ...)
{
	tag *tg = tags_add_empty(name);
	memset(tg->may_list, 0, sizeof(tg->may_list));
	tg->is_empty_tag = 0;

	int i = 0;
	char **tmp = &name;
	while (*(++tmp) != NULL)
	{
		tag *tmp_tg = tags_get(*tmp);
		if (tmp_tg == NULL) tmp_tg = tags_alloc(*tmp);
		tg->may_list[i++] = tmp_tg;
	}

	return tg;
}

// Заполняет список min_list. Если какого-либо тега не существовало,
// он будет создан. Последний параметр должен быть NULL.
tag * tags_add_min_list(char *name, ...)
{
	tag *tg = tags_add_empty(name);
	memset(tg->min_list, 0, sizeof(tg->min_list));
	tg->is_empty_tag = 0;

	int i = 0;
	char **tmp = &name;
	while (*(++tmp) != NULL)
	{
		tag *tmp_tg = tags_get(*tmp);
		if (tmp_tg == NULL) tmp_tg = tags_alloc(*tmp);
		tg->min_list[i++] = tmp_tg;
	}

	return tg;
}

// Заполняет список must_list. Если какого-либо тега не существовало,
// он будет создан. Последний параметр должен быть NULL.
tag * tags_add_must_list(char *name, ...)
{
	tag *tg = tags_add_empty(name);
	memset(tg->must_list, 0, sizeof(tg->must_list));
	tg->is_empty_tag = 0;

	int i = 0;
	char **tmp = &name;
	while (*(++tmp) != NULL)
	{
		tag *tmp_tg = tags_get(*tmp);
		if (tmp_tg == NULL) tmp_tg = tags_alloc(*tmp);
		tg->must_list[i++] = tmp_tg;
	}

	return tg;
}

void tags_init()
{
	all_tags_counter = 0;
	memset(all_tags, 0, sizeof(all_tags));

	#define INLINE_ELEMENTS	"samp", "abbr", "code", "object", "sup", "kbd", "button", "br", "strong", "a", "select", "textarea", "cite", "q", "dfn", "var", "span", "em", "acronym", "label", "input", "sub", "tt", "big", "b", "i", "img", "small"
	#define BLOCK_ELEMENTS	"h3", "pre", "h6", "ol", "div", "ul", "h2", "h5", "blockquote", "h1", "dl", "table", "address", "h4", "hr", "p", "fieldset"
	#define TAGS_TEMP_1		"script", "ins", "del", "bdo", "map"
	#define TAGS_TEMP_2		"script", "ins", "form", "del", "noscript"
	#define TAGS_TEMP_3		"form", "del", "script", "map", "ins", "bdo", "noscript"

	tags_add_may_list("a", TAGS_TEMP_1, "samp", "abbr", "code", "object", "sup", "kbd", "button", "br", "strong", "select", "textarea", "cite", "q", "dfn", "var", "span", "em", "acronym", "label", "input", "sub", "tt", "big", "b", "i", "img", "small", NULL);
	tags_add_may_list("abbr", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("acronym", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("address", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_empty("area");
	tags_add_may_list("b", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_empty("base");
	tags_add_may_list("bdo", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("big", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("blockquote", BLOCK_ELEMENTS, TAGS_TEMP_2, NULL);
	tags_add_may_list("body", BLOCK_ELEMENTS, TAGS_TEMP_2, NULL);
	tags_add_empty("br");
	tags_add_may_list("button", "samp", "abbr", "code", "ins", "h2", "del", "object", "em", "sup", "h4", "tt", "script", "map", "br", "strong", "h3", "div", "h5", "cite", "dl", "table", "q", "address", "dfn", "var", "ul", "span", "pre", "blockquote", "p", "hr", "h1", "sub", "bdo", "big", "noscript", "h6", "ol", "i", "img", "b", "acronym", "small", "kbd", NULL);
	tags_add_may_list("caption", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("cite", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("code", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_empty("col");
	tags_add_may_list("colgroup", "col", NULL);
	tags_add_may_list("dd", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, NULL);
	tags_add_may_list("del", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, NULL);
	tags_add_may_list("dfn", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("div", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, NULL);
	tags_add_must_list("dl", "dt", "dd", NULL);
	tags_add_may_list("dt", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("em", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("fieldset", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, "legend", NULL);
	tags_add_may_list("form", BLOCK_ELEMENTS, "noscript", "script", "ins", "del", NULL);
	tags_add_may_list("h1", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("h2", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("h3", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("h4", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("h5", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("h6", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	 tags_add_may_list("head", "meta", "style", "script", "object", "link", "base", NULL);
	 tags_add_must_list("head", "title", NULL);
	tags_add_empty("hr");
	tags_add_must_list("html", "head", "body", NULL);
	tags_add_may_list("i", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_empty("img");
	tags_add_empty("input");
	tags_add_may_list("ins", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, NULL);
	tags_add_may_list("kbd", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("label", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("legend", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("li", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, NULL);
	tags_add_empty("link");
	 tags_add_min_list("map", BLOCK_ELEMENTS, TAGS_TEMP_2, NULL);
	 tags_add_must_list("map", "area", NULL);
	tags_add_empty("meta");
	tags_add_may_list("noscript", BLOCK_ELEMENTS, TAGS_TEMP_2, NULL);
	tags_add_may_list("object", INLINE_ELEMENTS, BLOCK_ELEMENTS, "form", "del", "script", "map", "ins", "param", "bdo", "noscript", NULL);
	tags_add_min_list("ol", "li", NULL);
	tags_add_min_list("optgroup", "option", NULL);
	tags_add_may_list("option", NULL);
	tags_add_may_list("p", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_empty("param");
	tags_add_may_list("pre", TAGS_TEMP_1, "samp", "abbr", "code", "sup", "kbd", "button", "br", "strong", "a", "select", "textarea", "cite", "q", "dfn", "var", "span", "em", "acronym", "label", "input", "sub", "tt", "big", "b", "i", "small", NULL);
	tags_add_may_list("q", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("samp", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("script", NULL);
	tags_add_min_list("select", "optgroup", "option", NULL);
	tags_add_may_list("small", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("span", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("strong", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("style", NULL);
	tags_add_may_list("sub", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_may_list("sup", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	 tags_add_may_list("table", "caption", "thead", "tfoot", "colgroup", "col", NULL);
	 tags_add_min_list("table", "tr", "tbody", NULL);
	tags_add_min_list("tbody", "tr", NULL);
	tags_add_may_list("td", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, NULL);
	tags_add_may_list("textarea", NULL);
	tags_add_min_list("tfoot", "tr", NULL);
	tags_add_may_list("th", INLINE_ELEMENTS, BLOCK_ELEMENTS, TAGS_TEMP_3, NULL);
	tags_add_min_list("thead", "tr", NULL);
	tags_add_may_list("title", NULL);
	tags_add_min_list("tr", "td", "th", NULL);
	tags_add_may_list("tt", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
	tags_add_min_list("ul", "li", NULL);
	tags_add_may_list("var", INLINE_ELEMENTS, TAGS_TEMP_1, NULL);
}

void tags_free() {}
