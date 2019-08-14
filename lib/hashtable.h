#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <pthread.h>
#include <stdbool.h>


#define DEFAULT_SIZE	1013


/* type definition for a generic comparing function */
typedef int (*hash_table_compare_function)(const void *, const void *);
/* type definition for a for-each function */
typedef unsigned long (*hash_table_hash_function)(const void *);


typedef struct list_node {
	void * data;
	struct list_node * next;
} list_node_t;


typedef struct hash_table {
	int size;
	list_node_t * * elements;
	pthread_mutex_t * locks;
	pthread_mutex_t global_table_lock;
	hash_table_compare_function compare;	/* should return 0 if equal */
	hash_table_hash_function hash;
} hash_table_t;


hash_table_t *
hash_table_create(hash_table_compare_function, hash_table_hash_function, int max_size);


bool
hash_table_insert(hash_table_t * table, void * element);


bool
hash_table_contains(hash_table_t * table, void * element);


bool
hash_table_remove(hash_table_t * table, void * element);


void
hash_table_destroy(hash_table_t * table);


#endif /* HASHTABLE_H */
