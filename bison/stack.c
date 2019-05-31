#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include "y.tab.h"

typedef struct // Описывает атрибуты, содержащиеся у данного тега.
{
	attr *attribute;
	char *value;

	struct stack_attr *next;
} stack_attr;

typedef struct
{
	char tag_name[32];
	struct stack_elem *list_nested;			// Список тегов, содержащихся в данном.
	stack_attr *list_attributes;		// Список атрибутов данного тега.

	struct stack_elem *next;				// Для образования общего списка тегов.
} stack_elem;

stack_elem *head;

static stack_elem * stack_create_elem(char *tag_name);
static void stack_free_elem(stack_elem *elem);
static void stack_list_nested_push(stack_elem *elem, char *nested_name);
static void stack_attr_push(stack_elem *dest, attr *at, char *val);
static int stack_tag_count(char *tag);
static int stack_count();
static int stack_is_list_contains(stack_elem *list, char *tg_name);
static stack_elem * stack_get_elem_by_id(int id);
static void stack_push_check_standard(char *tag);
static void stack_pop_check_standard(char *tag);

void stack_init()
{
	head = NULL;
}

void stack_free()
{
	stack_elem *tmp = head, *tmp_2;
	while (tmp != NULL)
	{
		tmp_2 = tmp;
		tmp = tmp->next;
		free(tmp_2);
	}
	head = NULL;
}

// Помещает открывающий тег tag_name в стек.
void stack_push(char *tag_name)
{
	stack_push_check_standard(tag_name);

	stack_elem *elem = stack_create_elem(tag_name);

	if (head == NULL) head = elem;
	else
	{
		stack_elem *tmp = head;
		while (tmp)
			if (tmp->next == NULL)
			{
				tmp->next = elem;
				stack_list_nested_push(tmp, tag_name);
				break;
			} else
				tmp = tmp->next;
	}
}

// Удаляет элемент из стека, если открывающий тег совпадает с закрывающим.
// Иначе - ошибка.
void stack_pop(char *tag_name, int is_short_tag_form)
{
	stack_elem *last_elem = head, *pre_last_elem = head;
	if (head == NULL) { yyerror("stack_pop: stack is empty"); return; }

	while (last_elem->next != NULL)
	{
		pre_last_elem = last_elem;
		last_elem = last_elem->next;
	}

	if (!is_short_tag_form && strcmp(last_elem->tag_name, tag_name) != 0)
		yyerror("stack_pop: wrong closure tag");

	stack_pop_check_standard(tag_name, is_short_tag_form);

	pre_last_elem->next = NULL;
	if (last_elem == head) head = NULL;
	stack_free_elem(last_elem);
}

void stack_show()
{
	printf("\n- - -\nstack_show: output started\n");

	stack_elem *tmp = head;
	while (tmp)
	{
		printf(" %s", tmp->tag_name);

		if (tmp->list_attributes != NULL)
		{
			printf(" [");
			stack_attr *t = tmp->list_attributes;
			while (t)
			{
				printf(" %s", t->attribute->name);
				t = t->next;
			}
			printf(" ]");
		}

		if (tmp->list_nested == NULL)
			printf("\n");
		else
		{
			printf(": ");
			stack_elem *nested = tmp->list_nested;
			while (nested)
			{
				printf("%s ", nested->tag_name);
				nested = nested->next;
			}
			printf("\n");
		}

		tmp = tmp->next;
	}

	printf("stack_show: output ended\n- - -\n");
}

// На входе строка формата: name="val"
void stack_push_attribute(char *attr_name_val)
{
	stack_elem *last_elem = head, *pre_last_elem = head;
	if (head == NULL) { yyerror("stack_push_attribute: stack is empty"); return; }

	while (last_elem->next != NULL)
	{
		pre_last_elem = last_elem;
		last_elem = last_elem->next;
	}

	// Парсим имя и значение атрибута.
	char *attr_name = strtok(attr_name_val, "= \t\r\n");
	char *v = strtok(NULL, "= \t\r\n") + 1;
	v[strlen(v) - 1] = '\0';

	char *value = (char *) malloc(sizeof(char) * (strlen(v) + 1));
	sprintf(value, "%s", v);

	attr *at = attr_get(attr_name);
	if (at == NULL) { yyerror("stack_push_attribute: unknown attribute '%s'", attr_name); return; }

	if (!attr_is_allowed_for_tag(last_elem->tag_name, at))
		yyerror("stack_push_attribute: attribute '%s' is not allowed here", attr_name);
	else if (!attr_can_contains_value(last_elem->tag_name, at, value))
		yyerror("stack_push_attribute: attribute '%s' can't be '%s'", attr_name, value);
	else
		stack_attr_push(last_elem, at, value);
}

// -----------------------------------------------------------------------------

// Не управляет общим списком next. Только выделяет память.
static stack_elem * stack_create_elem(char *tag_name)
{
	stack_elem *elem = (stack_elem *) malloc(sizeof(stack_elem));
	if (elem == NULL) { yyerror("stack_create_elem: memory allocation error"); return NULL; }

	memset(elem, 0, sizeof(stack_elem));
	sprintf(elem->tag_name, "%s", tag_name);

	return elem;
}

// Не управляет общим списком next. Только очищает память.
static void stack_free_elem(stack_elem *elem)
{
	if (elem == NULL) return;

	// Очищаем список вложенных элементов.
	stack_elem *tmp, *ptmp;
s:	tmp = ptmp = elem->list_nested;
	while (tmp)
	{
		if (tmp->next == NULL)
		{
			ptmp->next = NULL;
			if (tmp == elem->list_nested)
				elem->list_nested = NULL;
			stack_free_elem(tmp);
			goto s;
		} else
		{
			ptmp = tmp;
			tmp = tmp->next;
		}
	}

	free(elem);
}

// Помещает тег с именем nested_name в список вложенных для тега elem.
static void stack_list_nested_push(stack_elem *elem, char *nested_name)
{
	if (elem == NULL) return;

	stack_elem *new = stack_create_elem(nested_name);
	stack_elem *tmp = elem->list_nested;
	if (tmp == NULL) { elem->list_nested = new; return; }

	while (tmp)
		if (tmp->next == NULL)
		{
			tmp->next = new;
			break;
		} else
			tmp = tmp->next;
}

static void stack_attr_push(stack_elem *dest, attr *at, char *val)
{
	if (dest == NULL || at == NULL || val == NULL) return;

	stack_attr *new = (stack_attr *) malloc(sizeof(stack_attr));
	memset(new, 0, sizeof(stack_attr));
	new->attribute = at;
	new->value = val;

	stack_attr *tmp = dest->list_attributes;
	if (tmp == NULL) { dest->list_attributes = new; return; }

	while (tmp)
		if (tmp->next == NULL)
		{
			tmp->next = new;
			break;
		} else
			tmp = tmp->next;
}

static int stack_tag_count(char *tag)
{
	int counter = 0;
	stack_elem *tmp = head;
	while (tmp)
	{
		if (strcmp(tag, tmp->tag_name) == 0)
			counter++;
		tmp = tmp->next;
	}
	return counter;
}

static int stack_count()
{
	int counter = 0;
	stack_elem *tmp = head;
	while (tmp)
	{
		counter++;
		tmp = tmp->next;
	}
	return counter;
}

// Проверяет, содержит ли список list элемент tg_name.
static int stack_is_list_contains(stack_elem *list, char *tg_name)
{
	while (list)
	{
		if (strcmp(list->tag_name, tg_name) == 0)
			return 1;
		list = list->next;
	}
	return 0;
}

static stack_elem * stack_get_elem_by_id(int id)
{
	int cid = 0;
	stack_elem *tmp = head;
	while (tmp)
	{
		if (cid == id) return tmp;
		tmp = tmp->next; cid++;
	}
	return NULL;
}

static int stack_is_contains_attribute(stack_attr *list, attr *pattr)
{
	if (list == NULL || pattr == NULL)
		return 0;

	while (list)
	{
		if (list->attribute == pattr)
			return 1;
		list = list->next;
	}

	return 0;
}

// Проверка соответствия иерархии тегов стандарту XHTML 1.0.
static void stack_pop_check_standard(char *tg_name, int is_short_tag_form)
{
	stack_elem *last_elem = stack_get_elem_by_id(stack_count() - 1);
	if (last_elem == NULL) yyerror("stack_pop_check_standard: stack is empty");

	tag *last_tg = tags_get(last_elem->tag_name);
	if (is_short_tag_form && last_tg->is_empty_tag)
		return;
	else if (is_short_tag_form && !last_tg->is_empty_tag)
		yyerror("stack_pop_check_standard: tag '%s' must have </%s> end tag form", last_tg->name, last_tg->name);
	else if (!is_short_tag_form && last_tg->is_empty_tag)
		yyerror("stack_pop_check_standard: tag '%s' must be empty and have <%s /> end tag form", last_tg->name, last_tg->name);

	tag *tg = tags_get(tg_name);
	if (tg == NULL) yyerror("stack_pop_check_standard: unknown tag '%s'", tg_name);

	{ // Проверка тега на наличие требуемых атрибутов.
		attr *at = list_attr;
		while (at)
		{
			attr_rule *rule = at->rule_list;
			while (rule)
			{
				if (rule->type != REQUIRED || !attr_is_rule_applicable_to_tag(rule, tg_name))
					goto next;

				if (!stack_is_contains_attribute(last_elem->list_attributes, at))
					yyerror("stack_pop_check_standard: tag '%s' must contain attribute '%s'", tg_name, at->name);

				next: rule = rule->next;
			}

			at = at->next;
		}
	}

	{ // Проверка min_list списка (см. объявление).
		int min_contains = (tg->min_list[0] == NULL);
		for (int i = 0; i < sizeof(tg->min_list) / sizeof(tg->min_list[0]); i++)
		{
			if (tg->min_list[i] == NULL) break;
			if (stack_is_list_contains(last_elem->list_nested, tg->min_list[i]->name))
			{ min_contains = 1; break; }
		}
		if (!min_contains)
			yyerror("stack_pop_check_standard: wrong internal <%s> tag structure", tg_name);
	}

	{ // Проверка must_list списка (см. объявление).
		for (int i = 0; i < sizeof(tg->must_list) / sizeof(tg->must_list[0]); i++)
		{
			if (tg->must_list[i] == NULL) break;
			if (!stack_is_list_contains(last_elem->list_nested, tg->must_list[i]->name))
				yyerror("stack_pop_check_standard: tag <%s> must contains tag %s", tg_name, tg->must_list[i]->name);
		}
	}
}

// Проверка соответствия иерархии тегов стандарту XHTML 1.0.
static void stack_push_check_standard(char *tg_name)
{
	tag *tg = tags_get(tg_name);
	if (tg == NULL) yyerror("stack_push_check_standard: unknown tag <%s>", tg_name);

	stack_elem *last_elem = stack_get_elem_by_id(stack_count() - 1);
	if (last_elem != NULL)
	{
		tag *tg = tags_get(last_elem->tag_name);
		if (tg == NULL) yyerror("stack_push_check_standard: unknown tag <%s>", tg_name);

		if (!tags_is_may_list_contains(tg, tg_name)
		&&	!tags_is_min_list_contains(tg, tg_name)
		&&	!tags_is_must_list_contains(tg, tg_name))
			yyerror("tag <%s> is not allowed inside tag <%s>", tg_name, tg->name);
	}
}
