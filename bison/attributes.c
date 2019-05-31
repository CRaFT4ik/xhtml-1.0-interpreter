/*
 * Coded by Eldar Timraleev (aka CRaFT4ik) © 2019.
 */
 
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <stdarg.h>
#include "y.tab.h"

#define NTAGS 100	// Макс. количество тегов.
#define ATTR_VAL_LEN 100

enum attr_rule_type
{
	ALLOWED,	// Правило разрешает использовать указанные теги.
	DENIED,		// Правило запрещает использовать указанные теги.
	EXCLUDED,	// Правило запрещает указанные теги и разрешает все остальные.
	REQUIRED	// Правило требует, что к указанным тегам был применен данный атрибут.
};

// Структура, описывающая правила для атрибута тега.
typedef struct st_attr_rule
{
	enum attr_rule_type type;			// Тип правила.
	tag **tag_list;						// Список тегов, к которым это правило применено.
	int allowed_values_cnt;
	char *allowed_values[15];			// Список разрешенных значений атрибута для данных тегов. \
										// Если список обнулен, разрешены все значения (все символы).
	struct st_attr_rule *next;			// Для образования связного списка.
} attr_rule;

// Структура, описывающая атрибут для тега.
typedef struct st_attr
{
	char name[32];						// Название атрибута.
	attr_rule *rule_list;				// Список правил атрибута.

	struct st_attr *next;				// Для образования связного списка.
} attr;

attr *list_attr;

static attr * attr_create(char *name)
{
	attr *elem = (attr *) malloc(sizeof(attr));
	if (elem == NULL) { yyerror("attr_create: memory allocation error"); return NULL; }

	memset(elem, 0, sizeof(attr));
	sprintf(elem->name, "%s", name);

	if (list_attr == NULL) list_attr = elem;
	else
	{
		attr *tmp = list_attr;
		while (tmp)
			if (tmp->next == NULL)
			{
				tmp->next = elem;
				break;
			} else
				tmp = tmp->next;
	}

	return elem;
}

// Последний параметр всегда NULL.
static tag ** attr_rule_create_tag_list(char *tg, ...)
{
	if (tg == NULL) return NULL;

	tag **p = (tag **) malloc(NTAGS * sizeof(tag *));
	memset(p, 0, sizeof(NTAGS * sizeof(tag *)));

	for (int i = 0; i < NTAGS; i++)
		p[i] = NULL;

	int i = 0;
	char **tmp = &tg;
	while (*tmp != NULL)
	{
		tag *tmp_tg = tags_get(*tmp);
		if (tmp_tg == NULL) yyerror("attr_rule_create_tag_list: unknown tag <%s>", tg);
		p[i++] = tmp_tg;
		tmp++;
	}

	return p;
}

static void attr_rule_push_allowed_value(attr_rule *rule, char *allowed_value)
{
	if (rule == NULL || allowed_value == NULL) return;

	if (rule->allowed_values_cnt >= sizeof(rule->allowed_values)/sizeof(rule->allowed_values[0]))
		yyerror("attr_rule_push_allowed_value: too small buffer for 'attr_rule->allowed_values'");

	if (ATTR_VAL_LEN <= strlen(allowed_value))
		yyerror("attr_rule_push_allowed_value: too small ATTR_VAL_LEN value");

	char *s = (char *) malloc(ATTR_VAL_LEN * sizeof(char));
	sprintf(s, "%s", allowed_value);

	rule->allowed_values[rule->allowed_values_cnt++] = s;
}

// Последний параметр всегда NULL.
static attr_rule * attr_create_rule(attr *a, enum attr_rule_type type, tag **tag_list, char *allowed_value, ...)
{
	assert(a != NULL);

	attr_rule *elem = (attr_rule *) malloc(sizeof(attr_rule));
	if (elem == NULL) { yyerror("attr_create_rule: memory allocation error"); return NULL; }

	memset(elem, 0, sizeof(attr_rule));
	elem->type = type;
	elem->tag_list = tag_list;

	if (allowed_value != NULL)
	{
		char **tmp = &allowed_value;
		while (*tmp != NULL)
		{
			attr_rule_push_allowed_value(elem, *tmp);
			tmp++;
		}
	}

	// Добавляем правило к атрибуту.

	attr_rule *rule_list = a->rule_list;
	if (rule_list == NULL) a->rule_list = elem;
	else
	{
		attr_rule *tmp = rule_list;
		while (tmp)
			if (tmp->next == NULL)
			{
				tmp->next = elem;
				break;
			} else
				tmp = tmp->next;
	}

	return NULL;
}

// Применимо ли правило rule для тега tg_name.
static int attr_is_rule_applicable_to_tag(attr_rule *rule, char *tg_name)
{
	assert(rule != NULL); assert(tg_name != NULL);

	enum attr_rule_type type = rule->type;

	if (rule->tag_list == NULL)
		if (type == ALLOWED || type == DENIED)
			// Разрешающее\запрещающее правило без тегов включает в себя любой тег.
			return 1;
		else if (type == EXCLUDED || type == REQUIRED)
			// Исключающее\требующее правило без тегов не включает никакой тег.
			return 0;
		else
			return 0;

	for (int i = 0; i < NTAGS; i++)
	{
		tag *tg_tmp = *(rule->tag_list + i);
		if (tg_tmp == NULL)
			return 0;

		if (strcmp(tg_tmp->name, tg_name) == 0)
			return 1;
	}

	return 0;
}

// Получить первое примененное правило атрибута at к тегу tg_name.
static attr_rule * attr_get_first_applied_rule(char *tg_name, attr *at)
{
	assert(tg_name != NULL); assert(at != NULL);

	attr_rule *rule = at->rule_list;
	while (rule)
	{
		enum attr_rule_type type = rule->type;
		int applicable = attr_is_rule_applicable_to_tag(rule, tg_name);

		if ((type == ALLOWED	&& applicable)
		 || (type == DENIED		&& applicable)
		 || (type == EXCLUDED	&& !applicable)
		 || (type == REQUIRED	&& applicable))
			return rule;

		rule = rule->next;
	}

	debug("[WARN] attr_get_first_applied_rule: there's no rule for tag '%s' and attribute '%s'\n", tg_name, at->name);
	return NULL;
}

// Проверка, содержит ли список allowed_values из правила rule значение value.
int attr_is_rule_values_list_contain(attr_rule *rule, char *value)
{
	assert(rule != NULL); assert(value != NULL);

	// Если список allowed_values пуст, то он содержит все значения.
	if (rule->allowed_values[0] == NULL)
		return 1;

	for (int i = 0; i < sizeof(rule->allowed_values)/sizeof(rule->allowed_values[0]); i++)
	{
		char *c_value = rule->allowed_values[i];
		if (c_value == NULL) break;

		if (strcmp(c_value, value) == 0)
			return 1;
	}

	return 0;
}

attr * attr_get(char *name)
{
	attr *tmp = list_attr;
	while (tmp)
	{
		if (strcmp(tmp->name, name) == 0)
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

// Проверка, может ли атрибут at иметь значение value.
int attr_can_contains_value(char *tg_name, attr *at, char *value)
{
	assert(tg_name != NULL); assert(at != NULL); assert(value != NULL);

	if (!attr_is_allowed_for_tag(tg_name, at)) return 0;

	attr_rule *rule = attr_get_first_applied_rule(tg_name, at);
	assert(rule != NULL);

	if (attr_is_rule_values_list_contain(rule, value))
		return 1;

	return 0;
}

// Проверка, может ли тег tg_name содержать атрибут at.
int attr_is_allowed_for_tag(char *tg_name, attr *at)
{
	assert(tg_name != NULL); assert(at != NULL);

	attr_rule *rule = attr_get_first_applied_rule(tg_name, at);
	if (rule == NULL)
		return 0;

	enum attr_rule_type type = rule->type;
	if (type == ALLOWED || type == EXCLUDED || type == REQUIRED)
		return 1;

	return 0;
}

void attr_init()
{
	list_attr = NULL;
	attr *a; tag **l;

	a = attr_create("xml:lang");
	l = attr_rule_create_tag_list("base", "br", "param", "script", NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("xmlns");
	l = attr_rule_create_tag_list("html", NULL);
	attr_create_rule(a, REQUIRED, l, "http://www.w3.org/1999/xhtml", NULL);

	a = attr_create("class");
	l = attr_rule_create_tag_list("head", "title", "script", "style", "param", "base", "meta", "html", NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("id");
	l = attr_rule_create_tag_list("map", NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);
	l = attr_rule_create_tag_list("map", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("title");
	l = attr_rule_create_tag_list("head", "title", "script", "html", "param", "meta", "base", NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("xml:space");
	l = attr_rule_create_tag_list("pre", "style", "script", NULL);
	attr_create_rule(a, ALLOWED, l, "preserve", NULL);

	a = attr_create("style");
	l = attr_rule_create_tag_list("head", "title", "script", "style", "param", "base", "meta", "html", NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	#define SET_1 "head", "title", "script", "style", "param", "base", "meta", "br", "html"

	a = attr_create("onclick");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("ondblclick");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("onkeydown");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("onkeypress");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("onmousedown");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("onmousemove");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("onmouseout");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("onmouseover");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("onmouseup");
	l = attr_rule_create_tag_list(SET_1, NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("abbr");
	l = attr_rule_create_tag_list("td", "th", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("accept");
	l = attr_rule_create_tag_list("input", "form", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("accept-charset");
	l = attr_rule_create_tag_list("form", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("accesskey");
	l = attr_rule_create_tag_list("a", "textarea", "area", "button", "label", "input", "legend", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("action");
	l = attr_rule_create_tag_list("form", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("align");
	l = attr_rule_create_tag_list("colgroup", "tr", "tbody", "tfoot", "th", "td", "col", "thead", NULL);
	attr_create_rule(a, ALLOWED, l, "left", "center", "right", "justify", "char", NULL);

	a = attr_create("alt");
	l = attr_rule_create_tag_list("input", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);
	l = attr_rule_create_tag_list("img", "area", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("archive");
	l = attr_rule_create_tag_list("object", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("axis");
	l = attr_rule_create_tag_list("td", "th", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("border");
	l = attr_rule_create_tag_list("table", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("cellpadding");
	l = attr_rule_create_tag_list("table", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("cellspacing");
	l = attr_rule_create_tag_list("table", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("char");
	l = attr_rule_create_tag_list("colgroup", "tr", "tbody", "tfoot", "th", "td", "col", "thead", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("charoff");
	l = attr_rule_create_tag_list("colgroup", "tr", "tbody", "tfoot", "th", "td", "col", "thead", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("charset");
	l = attr_rule_create_tag_list("a", "link", "script", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("checked");
	l = attr_rule_create_tag_list("input", NULL);
	attr_create_rule(a, ALLOWED, l, "checked", NULL);

	a = attr_create("cite");
	l = attr_rule_create_tag_list("q", "blockquote", "del", "ins", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("classid");
	l = attr_rule_create_tag_list("object", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("codebase");
	l = attr_rule_create_tag_list("object", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("codetype");
	l = attr_rule_create_tag_list("object", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("cols");
	l = attr_rule_create_tag_list("textarea", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("colspan");
	l = attr_rule_create_tag_list("td", "th", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("content");
	l = attr_rule_create_tag_list("meta", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("coords");
	l = attr_rule_create_tag_list("a", "area", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("data");
	l = attr_rule_create_tag_list("object", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("datetime");
	l = attr_rule_create_tag_list("del", "ins", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("declare");
	l = attr_rule_create_tag_list("object", NULL);
	attr_create_rule(a, ALLOWED, l, "declare", NULL);

	a = attr_create("defer");
	l = attr_rule_create_tag_list("script", NULL);
	attr_create_rule(a, ALLOWED, l, "defer", NULL);

	a = attr_create("dir");
	l = attr_rule_create_tag_list("bdo", "base", "br", "param", "script", NULL);
	attr_create_rule(a, EXCLUDED, l, "ltr", "rtl", NULL);
	l = attr_rule_create_tag_list("bdo", NULL);
	attr_create_rule(a, REQUIRED, l, "ltr", "rtl", NULL);

	a = attr_create("disabled");
	l = attr_rule_create_tag_list("option", "textarea", "button", "optgroup", "input", "select", NULL);
	attr_create_rule(a, ALLOWED, l, "disabled", NULL);

	a = attr_create("enctype");
	l = attr_rule_create_tag_list("form", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("for");
	l = attr_rule_create_tag_list("label", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("frame");
	l = attr_rule_create_tag_list("table", NULL);
	attr_create_rule(a, ALLOWED, l, "void", "above", "below", "hsides", "lhs", "rhs", "vsides", "box", "border", NULL);

	a = attr_create("headers");
	l = attr_rule_create_tag_list("td", "th", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("height");
	l = attr_rule_create_tag_list("object", "img", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("href");
	l = attr_rule_create_tag_list("base", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);
	l = attr_rule_create_tag_list("a", "link", "area", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("hreflang");
	l = attr_rule_create_tag_list("a", "link", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("http-equiv");
	l = attr_rule_create_tag_list("meta", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("ismap");
	l = attr_rule_create_tag_list("img", NULL);
	attr_create_rule(a, ALLOWED, l, "ismap", NULL);

	a = attr_create("label");
	l = attr_rule_create_tag_list("option", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);
	l = attr_rule_create_tag_list("optgroup", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("lang");
	l = attr_rule_create_tag_list("base", "br", "param", "script", NULL);
	attr_create_rule(a, EXCLUDED, l, NULL);

	a = attr_create("longdesc");
	l = attr_rule_create_tag_list("img", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("maxlength");
	l = attr_rule_create_tag_list("input", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("media");
	l = attr_rule_create_tag_list("style", "link", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("method");
	l = attr_rule_create_tag_list("form", NULL);
	attr_create_rule(a, ALLOWED, l, "get", "post", NULL);

	a = attr_create("multiple");
	l = attr_rule_create_tag_list("select", NULL);
	attr_create_rule(a, ALLOWED, l, "multiple", NULL);

	a = attr_create("name");
	l = attr_rule_create_tag_list("textarea", "button", "param", "meta", "input", "select", "a", "map", "object", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("nohref");
	l = attr_rule_create_tag_list("area", NULL);
	attr_create_rule(a, ALLOWED, l, "nohref", NULL);

	a = attr_create("onblur");
	l = attr_rule_create_tag_list("a", "textarea", "area", "button", "label", "input", "select", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("onchange");
	l = attr_rule_create_tag_list("input", "textarea", "select", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("onfocus");
	l = attr_rule_create_tag_list("a", "textarea", "area", "button", "label", "input", "select", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("onload");
	l = attr_rule_create_tag_list("body", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("onreset");
	l = attr_rule_create_tag_list("form", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("onselect");
	l = attr_rule_create_tag_list("input", "textarea", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("onsubmit");
	l = attr_rule_create_tag_list("form", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("onunload");
	l = attr_rule_create_tag_list("body", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("profile");
	l = attr_rule_create_tag_list("head", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("readonly");
	l = attr_rule_create_tag_list("input", "textarea", NULL);
	attr_create_rule(a, ALLOWED, l, "readonly", NULL);

	a = attr_create("rel");
	l = attr_rule_create_tag_list("a", "link", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("rev");
	l = attr_rule_create_tag_list("a", "link", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("rows");
	l = attr_rule_create_tag_list("textarea", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("rowspan");
	l = attr_rule_create_tag_list("td", "th", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("rules");
	l = attr_rule_create_tag_list("table", NULL);
	attr_create_rule(a, ALLOWED, l, "none", "groups", "rows", "cols", "all", NULL);

	a = attr_create("scheme");
	l = attr_rule_create_tag_list("meta", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("scope");
	l = attr_rule_create_tag_list("td", "th", NULL);
	attr_create_rule(a, ALLOWED, l, "row", "col", "rowgroup", "colgroup", NULL);

	a = attr_create("selected");
	l = attr_rule_create_tag_list("option", NULL);
	attr_create_rule(a, ALLOWED, l, "selected", NULL);

	a = attr_create("shape");
	l = attr_rule_create_tag_list("a", "area", NULL);
	attr_create_rule(a, ALLOWED, l, "rect", "circle", "poly", "default", NULL);

	a = attr_create("size");
	l = attr_rule_create_tag_list("input", "select", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("span");
	l = attr_rule_create_tag_list("col", "colgroup", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("src");
	l = attr_rule_create_tag_list("input", "script", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);
	l = attr_rule_create_tag_list("img", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);

	a = attr_create("standby");
	l = attr_rule_create_tag_list("object", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("summary");
	l = attr_rule_create_tag_list("table", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("tabindex");
	l = attr_rule_create_tag_list("a", "textarea", "area", "button", "object", "input", "select", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("type");
	l = attr_rule_create_tag_list("style", "script", NULL);
	attr_create_rule(a, REQUIRED, l, NULL);
	l = attr_rule_create_tag_list("button", NULL);
	attr_create_rule(a, ALLOWED, l, "button", "submit", "reset", NULL);
	l = attr_rule_create_tag_list("a", "object", "link", "param", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);
	l = attr_rule_create_tag_list("input", NULL);
	attr_create_rule(a, ALLOWED, l, "text", "password", "checkbox", "radio", "submit", "reset", "file", "hidden", "image", "button", NULL);

	a = attr_create("usemap");
	l = attr_rule_create_tag_list("input", "object", "img", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("valign");
	l = attr_rule_create_tag_list("colgroup", "tr", "tbody", "tfoot", "th", "td", "col", "thead", NULL);
	attr_create_rule(a, ALLOWED, l, "top", "middle", "bottom", "baseline", NULL);

	a = attr_create("value");
	l = attr_rule_create_tag_list("input", "button", "option", "param", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);

	a = attr_create("valuetype");
	l = attr_rule_create_tag_list("param", NULL);
	attr_create_rule(a, ALLOWED, l, "data", "ref", "object", NULL);

	a = attr_create("width");
	l = attr_rule_create_tag_list("table", "object", "col", "img", "colgroup", NULL);
	attr_create_rule(a, ALLOWED, l, NULL);
}

void attr_free()
{
	attr *at = list_attr, *tmp_at;
	while (at)
	{
		attr_rule *rule = at->rule_list, *tmp_rule;
		while (rule)
		{
			if (rule->tag_list != NULL)
				free(rule->tag_list);

			for (int i = 0; i < sizeof(rule->allowed_values)/sizeof(rule->allowed_values[0]); i++)
				if (rule->allowed_values[i] != NULL) free(rule->allowed_values[i]);

			tmp_rule = rule->next;
			free(rule);
			rule = tmp_rule;
		}

		tmp_at = at->next;
		free(at);
		at = tmp_at;
	}
}
