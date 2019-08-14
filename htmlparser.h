
#include <stdbool.h>

typedef struct text_result {
	char * text;
	struct text_result * next;
} text_result_t;


text_result_t *
find_text(char * html_code);


bool
find_in_text(char * expr, text_result_t * result);


void
free_text_results(text_result_t * head);
