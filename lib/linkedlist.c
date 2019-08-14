/**
 * Thread-safe linked-list data-structure for C.
 *
 * See `../README.md` and `main()` in this file for usage.
 *
 * @file linked_list.c contains the implementatons of the functions outlined in `linked_list.h` as well as
 * all the functions necessary to manipulate and handle nodes (which are not exposed to
 * the user).
 *
 * @author r-medina
 *
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 r-medina
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "linkedlist.h"

/* macros */

// for locking and unlocking rwlocks along with `locktype_t`
#define RWLOCK(lt, lk) ((lt) == l_read)				   \
						   ? pthread_rwlock_rdlock(&(lk)) \
						   : pthread_rwlock_wrlock(&(lk))
#define RWUNLOCK(lk) pthread_rwlock_unlock(&(lk));

/* type definitions */

typedef enum locktype locktype_t;

// locktype enumerates the two typs of rw locks. This isused in the macros above for
// simplifying alinked_list the locking/unlocking that goes on.
enum locktype {
	l_read,
	l_write
};

// linked_list_node models a linked-list node
struct linked_list_node {
	// pointer to the value at the node
	void *val;

	// pointer to the next node
	linked_list_node_t *nxt;

	// rw mutex
	pthread_rwlock_t m;
};

/**
 * @function linked_list_new
 *
 * Alinked_listocates a new linked list and initalizes its values.
 *
 * @param val_teardown - the `val_teardown` attribute of the linked list will be set to this
 *
 * @returns a pointer to a new linked list
 */
linked_list_t *
linked_list_new(gen_fun_t val_teardown) {
	linked_list_t *list = (linked_list_t *)malloc(sizeof(linked_list_t));
	list->hd = NULL;
	list->len = 0;
	list->val_teardown = val_teardown;
	pthread_rwlock_init(&list->m, NULL);

	return list;
}

/**
 * @function linked_list_delete
 *
 * Traverses the whole linked list and deletes/dealinked_listocates the nodes then frees the linked
 * list itself.
 *
 * @param list - the linked list
 */
void
linked_list_delete(linked_list_t *list) {
	linked_list_node_t *node = list->hd;
	linked_list_node_t *tmp;
	RWLOCK(l_write, list->m);
	while (node != NULL) {
		RWLOCK(l_write, node->m);
		list->val_teardown(node->val);
		RWUNLOCK(node->m);
		tmp = node;
		node = node->nxt;
		pthread_rwlock_destroy(&(tmp->m));
		free(tmp);
		(list->len)--;
	}
	list->hd = NULL;
	list->val_teardown = NULL;
	list->val_printer = NULL;
	RWUNLOCK(list->m);

	pthread_rwlock_destroy(&(list->m));

	free(list);
}

/**
 * @function linked_list_new_node
 *
 * Makes a new node with the given value.
 *
 * @param val - a pointer to the value
 *
 * @returns a pointer to the new node
 */
linked_list_node_t *
linked_list_new_node(void * val) {
	linked_list_node_t * node = (linked_list_node_t *)malloc(sizeof(linked_list_node_t));
	node->val = val;
	node->nxt = NULL;
	pthread_rwlock_init(&node->m, NULL);

	return node;
}

/**
 * @function linked_list_select_n_min_1
 *
 * Actually selects the n - 1th element. Inserting and deleting at the front of a
 * list do NOT really depend on this.
 *
 * @param list - the linked list
 * @param node - a pointer to set when the node is found
 * @param n - the index
 *
 * @returns 0 if successful, -1 otherwise
 */
int
linked_list_select_n_min_1(linked_list_t *list, linked_list_node_t **node, int n, locktype_t lt) {
	if (n < 0) // don't check against list->len because threads can add length
		return -1;

	if (n == 0)
		return 0;

	// n > 0

	*node = list->hd;
	if (*node == NULL) // if head is NULL, but we're trying to go past it,
		return -1;	 // we have a problem

	RWLOCK(lt, (*node)->m);

	linked_list_node_t *last;
	for (; n > 1; n--) {
		last = *node;
		*node = last->nxt;
		if (*node == NULL) { // happens when another thread deletes the end of a list
			RWUNLOCK(last->m);
			return -1;
		}

		RWLOCK(lt, (*node)->m);
		RWUNLOCK(last->m);
	}

	return 0;
}

/**
 * @function linked_list_insert_n
 *
 * Inserts a value at the nth position of a linked list.
 *
 * @param list - the linked list
 * @param val - a pointer to the value
 * @param n - the index
 *
 * @returns 0 if successful, -1 otherwise
 */
int
linked_list_insert_n(linked_list_t *list, void *val, int n) {
	linked_list_node_t *new_node = linked_list_new_node(val);

	if (n == 0) { // nth_node is list->hd
		RWLOCK(l_write, list->m);
		new_node->nxt = list->hd;
		list->hd = new_node;
		RWUNLOCK(list->m);
	} else {
		linked_list_node_t *nth_node;
		if (linked_list_select_n_min_1(list, &nth_node, n, l_write)) {
			free(new_node);
			return -1;
		}
		new_node->nxt = nth_node->nxt;
		nth_node->nxt = new_node;
		RWUNLOCK(nth_node->m);
	}

	RWLOCK(l_write, list->m);
	(list->len)++;
	RWUNLOCK(list->m);

	return list->len;
}

/**
 * @function linked_list_insert_first
 *
 * Just a wrapper for `linked_list_insert_n` called with 0.
 *
 * @param list - the linked list
 * @param val - a pointer to the value
 *
 * @returns the new length of thew linked list on success, -1 otherwise
 */
int
linked_list_insert_first(linked_list_t *list, void *val) {
	return linked_list_insert_n(list, val, 0);
}

/**
 * @function linked_list_insert_last
 *
 * Just a wrapper for `linked_list_insert_n` called with the index being the length of the linked list.
 *
 * @param list - the linked list
 * @param val - a pointer to the value
 *
 * @returns the new length of thew linked list on success, -1 otherwise
 */
int
linked_list_insert_last(linked_list_t *list, void *val) {
	return linked_list_insert_n(list, val, list->len);
}

/**
 * @function linked_list_remove_n
 *
 * Removes the nth element of the linked list.
 *
 * @param list - the linked list
 * @param n - the index
 *
 * @returns the new length of thew linked list on success, -1 otherwise
 */
int
linked_list_remove_n(linked_list_t *list, int n) {
	linked_list_node_t *tmp;
	if (n == 0) {
		RWLOCK(l_write, list->m);
		tmp = list->hd;
		list->hd = tmp->nxt;
	} else {
		linked_list_node_t *nth_node;
		if (linked_list_select_n_min_1(list, &nth_node, n, l_write)) // if that node doesn't exist
			return -1;

		tmp = nth_node->nxt;
		nth_node->nxt = nth_node->nxt == NULL ? NULL : nth_node->nxt->nxt;
		RWUNLOCK(nth_node->m);
		RWLOCK(l_write, list->m);
	}

	(list->len)--;
	RWUNLOCK(list->m);

	list->val_teardown(tmp->val);
	free(tmp);

	return list->len;
}

/**
 * @function linked_list_remove_first
 *
 * Wrapper for `linked_list_remove_n`.
 *
 * @param list - the linked list
 *
 * @returns 0 if successful, -1 otherwise
 */
int
linked_list_remove_first(linked_list_t *list) {
	return linked_list_remove_n(list, 0);
}

/**
 * @function linked_list_remove_search
 *
 * Removes the first item in the list whose value returns 1 if `cond` is called on it.
 *
 * @param list - the linked list
 * @param cond - a function that will be called on the values of each node. It should
 * return 1 of the element matches.
 *
 * @returns the new length of thew linked list on success, -1 otherwise
 */
int
linked_list_remove_search(linked_list_t *list, int cond(void *)) {
	linked_list_node_t *last = NULL;
	linked_list_node_t *node = list->hd;
	while ((node != NULL) && !(cond(node->val))) {
		last = node;
		node = node->nxt;
	}

	if (node == NULL) {
		return -1;
	} else if (node == list->hd) {
		RWLOCK(l_write, list->m);
		list->hd = node->nxt;
		RWUNLOCK(list->m);
	} else {
		RWLOCK(l_write, last->m);
		last->nxt = node->nxt;
		RWUNLOCK(last->m);
	}

	list->val_teardown(node->val);
	free(node);

	RWLOCK(l_write, list->m);
	(list->len)--;
	RWUNLOCK(list->m);

	return list->len;
}

/**
 * @function linked_list_get_n
 *
 * Gets the value of the nth element of a linked list.
 *
 * @param list - the linked list
 * @param n - the index
 *
 * @returns the `val` attribute of the nth element of `list`.
 */
void *
linked_list_get_n(linked_list_t *list, int n) {
	linked_list_node_t *node;
	if (linked_list_select_n_min_1(list, &node, n + 1, l_read))
		return NULL;

	RWUNLOCK(node->m);
	return node->val;
}

/**
 * @function linked_list_get_first
 *
 * Wrapper for `linked_list_get_n`.
 *
 * @param list - the linked list
 *
 * @returns the `val` attribute of the first element of `list`.
 */
void *
linked_list_get_first(linked_list_t *list) {
	return linked_list_get_n(list, 0);
}

/**
 * @function linked_list_map
 *
 * Calinked_lists a function on the value of every element of a linked list.
 *
 * @param list - the linked list
 * @param f - the function to calinked_list on the values.
 */
void
linked_list_map(linked_list_t *list, gen_fun_t f) {
	linked_list_node_t *node = list->hd;

	while (node != NULL) {
		RWLOCK(l_read, node->m);
		f(node->val);
	linked_list_node_t *old_node = node;
		node = node->nxt;
		RWUNLOCK(old_node->m);
	}
}
