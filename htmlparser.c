#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "htmlparser.h"


#define	BUFSIZE	1638400


int
clear_whitespace(char * str, int i)
{
	while (str[i] == ' ' || str[i] == '\n')
		i++;

	return i;
}


text_result_t *
find_text(char * html_code)
{
	char buffer[BUFSIZE];
	int i = 0, j;
	text_result_t * current = NULL, * head = NULL;

	i = clear_whitespace(html_code, i);

	while (html_code[i++] != '\0') {

		while (html_code[i++] != '>');

		i = clear_whitespace(html_code, i);

		if (html_code[i] == '<')
			continue;

		i = clear_whitespace(html_code, i);

		if (html_code[i] == '\0')
			break;

		j = 0;
		while (html_code[i] != '<')
			buffer[j++] = html_code[i++];
		buffer[j] = '\0';

		text_result_t * result = malloc(sizeof(text_result_t));
		result->next = NULL;
		result->text = malloc((strlen(buffer) + 1) * sizeof(char));
		strcpy(result->text, buffer);

		if (current) {
			current->next = result;
			current = result;
		} else {
			head = result;
			current = result;
		}
	}

	return head;
}


bool
find_in_text(char * expr, text_result_t * result)
{
	text_result_t * iter;

	for (iter = result; iter != NULL; iter = iter->next)
		if (strstr(iter->text, expr))
			return true;

	return false;
}


void
free_text_results(text_result_t * head)
{
	text_result_t * aux;

	while(head) {
		aux = head;
		head = head->next;
		free(aux->text);
		free(aux);
	}
}

