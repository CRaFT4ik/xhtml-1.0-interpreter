#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include "y_tab.h"

void stack_push_check_standard(char *tag);

typedef struct
{
	char tag_name[32];
	struct stack_elem *next;
} stack_elem;

stack_elem *head;

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

void stack_push(char *tag_name)
{
	stack_push_check_standard(tag_name);

	stack_elem *elem = (stack_elem *) malloc(sizeof(stack_elem));
	if (elem == NULL) { yyerror("stack_push: memory allocation error"); return; }

	memcpy(elem->tag_name, tag_name, sizeof(char) * (strlen(tag_name) + 1));
	elem->next = NULL;

	if (head == NULL) head = elem;
	else
	{
		stack_elem *tmp = head;
		while (tmp)
		{
			if (tmp->next == NULL)
			{
				tmp->next = elem;
				break;
			} else
				tmp = tmp->next;
		}
	}
}

void stack_pop(char *tag_name, int is_short_tag_form)
{
	stack_elem *last_elem = head, *pre_last_elem = head;
	if (head == NULL) { yyerror("stack_pop: stack is empty"); return; }

	while (last_elem->next != NULL)
	{
		pre_last_elem = last_elem;
		last_elem = last_elem->next;
	}

	if (!is_short_tag_form)
	{
		char for_cmp[32 + 3];
		sprintf(for_cmp, "</%s>", last_elem->tag_name);
		if (strcmp(for_cmp, tag_name) != 0)
			yyerror("stack_pop: wrong closure tag");
	}

	pre_last_elem->next = NULL;
	if (last_elem == head) head = NULL;
	free(last_elem);
}

void stack_show()
{
	printf("stack_print: output started\n");

	stack_elem *tmp = head;
	while (tmp)
	{
		printf("%s\n", tmp->tag_name);
		tmp = tmp->next;
	}

	printf("stack_print: output ended\n");
}

// -----------------------------------------------------------------------------

int stack_tag_count(char *tag)
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

int stack_count()
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

stack_elem * stack_get_elem_by_id(int id)
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

// Проверка соответствия иерархии тегов стандарту XHTML 1.0.
void stack_push_check_standard(char *tg_name)
{
	tag *tg = tags_get(tg_name);
	if (tg == NULL) yyerror("stack_push_check_standard: unknown tag %s", tg_name);

	/*stack_elem *stel;
	for (int i = 0; i < stack_count(); i++)
	{
		stel = stack_get_elem_by_id(i);
	}*/

	stack_elem *last_elem = stack_get_elem_by_id(stack_count() - 1);
	if (last_elem != NULL)
	{
		tag *tg = tags_get(last_elem->tag_name);
		if (tg == NULL) yyerror("stack_push_check_standard: unknown tag %s", tg_name);

		if (!tags_is_may_list_contains(tg, tg_name))
			yyerror("tag %s is not allowed here", tg_name);
	}

	return;

	if (strcmp("a", tg_name) == 0)
	{
		if (stack_tag_count(tg_name) > 0)
			yyerror("tag <a> can't be recursively nested");
		return;
	}

	if (strcmp("html", tg_name) == 0)
	{
		if (stack_count() > 0)
			yyerror("tag <html> can't be nested");
		return;
	}

	if (strcmp("head", tg_name) == 0)
	{
		if (!(stack_count() == 1 && stack_tag_count("html") == 1))
			yyerror("use tag <head> after tag <html>");
		return;
	}

	if (strcmp("title", tg_name) == 0)
	{
		if (!(stack_count() == 2 && stack_tag_count("html") == 1 && stack_tag_count("head") == 1))
			yyerror("use tag <title> after tag <head>");
		return;
	}

	if (strcmp("body", tg_name) == 0)
	{
		if (!(stack_count() == 1 && stack_tag_count("html") == 1))
			yyerror("use tag <body> after tag <head>");
		return;
	}
}