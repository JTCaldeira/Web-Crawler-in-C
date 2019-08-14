#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <curl/curl.h>

#include "lib/hashtable.h"
#include "lib/linkedlist.h"
#include "lib/queue.h"
#include "htmlparser.h"


//#define	NUM_CORES	get_nprocs_conf()
#define	NUM_CORES	8
#define QUEUE_CAPACITY	16384
#define	MAX_DELAY		5000000	// 5 seconds


typedef struct memstruct {
	char * memory;
	size_t size;
} memstruct_t;


// gcc -Wall -Wextra -ggdb3 -g -std=gnu99 -pthread -o test crawler.c lib/*.c -lm -lcurl
// valgrind -v --leak-check=full --show-leak-kinds=all --track-origins=yes ./test


hash_table_t * table;
linked_list_t * results;
queue_t * work_queue;


char *
parse_args(int argc, char * argv[])
{
	char * url;

	if (argc < 3) {
		fprintf(stderr, "Invalid number of arguments.\n");
		exit(1);
	}

	url = malloc((strlen(argv[1]) + 1) * sizeof(char));
	strcpy(url, argv[1]);

	return url;
}


char *
parse_expr(int argc, char * argv[])
{
	int i, query_length = 0;
	char * expression = NULL, * aux;

	for (i = 1; i < argc; i++)
		query_length += strlen(argv[i]) + 1;	/* stores query length including spaces and \0 */

	expression = calloc(query_length, sizeof(char));

	aux = stpcpy(expression, argv[2]);
	for (i = 3; i < argc; i++) {
		aux = stpcpy(aux, " ");
		aux = stpcpy(aux, argv[i]);
	}
	expression[query_length-1] = '\0';

	return expression;
}


unsigned long
str_hash_function(const void * a)
{
	unsigned long hash = 5381;
	int c;
	char * str = (char*)a;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}


static size_t
write_mem(void * contents, size_t size, size_t nmemb, void * userp)
{
	size_t real_size = size * nmemb;
	memstruct_t * mem = (memstruct_t *)userp;

	char * ptr = realloc(mem->memory, mem->size + real_size + 1);

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, real_size);
	mem->size += real_size;
	mem->memory[mem->size] = 0;

	return real_size;
}


useconds_t
exponential_backoff(int useconds)
{
	usleep(useconds);

	useconds *= 2;

	if (useconds > MAX_DELAY)
		return 0;
	
	return useconds;
}


void *
do_work(void * data)
{
	char * url, * expr = (char*)data;
	useconds_t delay = 1000;

	CURL * curl_handle;
	CURLcode res;
	memstruct_t chunk;

	curl_handle = curl_easy_init();

	while (1) {
		if (!(queue_trypop(work_queue, (void**)&url))) {
			delay = exponential_backoff(delay);

			if (delay == 0)
				break;

			continue;
		} else {
			delay = 1000;

			if (hash_table_contains(table, url))
				continue;
			else
				hash_table_insert(table, url);
		}

		chunk.memory = malloc(1);
		chunk.size = 0;

		// specify URL to get
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		// send all data do this function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_mem);
		// pass the chunk struct to the callback function
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
		// some servers don't like requests that are made without a user-agent
		// field, so we provide one
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		res = curl_easy_perform(curl_handle);

		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed with url %s: %s\n", url, curl_easy_strerror(res));

		text_result_t * result = find_text(chunk.memory);

		if (find_in_text(expr, result)) {
			linked_list_insert_last(results, (void*)url);
			free(chunk.memory);
			free_text_results(result);
			break;
		}

		free(chunk.memory);
		free_text_results(result);
	}

	curl_easy_cleanup(curl_handle);

	return NULL;
}


void
create_workers(char * expression)
{
	pthread_t threads[NUM_CORES];
	int i;

	curl_global_init(CURL_GLOBAL_ALL);

	for (i = 0; i < NUM_CORES; i++)
		pthread_create(&threads[i], NULL, do_work, (void*)expression);

	for (i = 0; i < NUM_CORES; i++)
		pthread_join(threads[i], NULL);

	curl_global_cleanup();
}


// valgrind -v --leak-check=full --show-leak-kinds=all --track-origins=yes ./test https://this-page-intentionally-left-blank.org/ blank


void
print_result(void * value)
{
	static int i = 1;
	char * url = (char*)value;

	printf("\n%d: %s\n", i, url);
	i++;
}

int
main(int argc, char * argv[])
{
	char * url = parse_args(argc, argv);
	char * expression = parse_expr(argc, argv);

	// initialize data structures
	table = hash_table_create((hash_table_compare_function)strcmp, str_hash_function, -1);
	results = linked_list_new(free);
	queue_create(&work_queue, QUEUE_CAPACITY);

	queue_push(work_queue, url);

	// do multithreaded work
	create_workers(expression);

	// show the results
	linked_list_map(results, print_result);

	// free memory allocated by data structures
	free(expression);
	hash_table_destroy(table);
	queue_destroy(work_queue);
	linked_list_delete(results);

	return EXIT_SUCCESS;
}