/**
 * Thread-safe linked-list data-structure for C.
 *
 * See `../README.md` and `main()` in `linked_list.c` for usage.
 *
 * @file linked_list.h contains the API for using this data-structure.
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

#ifndef LINKED_LIST_H
#define LINKED_LIST_H


#include <pthread.h>

/* type definitions */

// useful for casting
typedef void (*gen_fun_t)(void *);

// linked list
typedef struct linked_list linked_list_t;

// linked list node
typedef struct linked_list_node linked_list_node_t;

// linked list
struct linked_list {
    // running length
    int len;

    // pointer to the first node
    linked_list_node_t *hd;

    // mutex for thread safety
    pthread_rwlock_t m;

    // a function that is called every time a value is deleted
    // with a pointer to that value
    gen_fun_t val_teardown;

    // a function that can print the values in a linked list
    gen_fun_t val_printer;
};

/* function prototypes */

// returns a pointer to an allocated linked list.
// needs a taredown function that is called with
// a pointer to the value when it is being deleted.
linked_list_t *
linked_list_new(gen_fun_t val_teardown);

// traverses the linked list, deallocated everything (including `list`)
void
linked_list_delete(linked_list_t * list);

// inserts a value into the linked list at position `n`. acceptable values for n are `0`
// (puts it in first) to `list->len` (puts it in last).
// returns the new length of the linked list if successful, -1 otherwise
int
linked_list_insert_n(linked_list_t * list, void * val, int n);

// puts a value at the front of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int
linked_list_insert_first(linked_list_t * list, void * val);

// puts a value at the end of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int
linked_list_insert_last(linked_list_t * list, void * val);

// removes the value at position n of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int
linked_list_remove_n(linked_list_t * list, int n);

// removes the value at the front of the linked list.
// returns the new length of the linked list if successful, -1 otherwise
int
linked_list_remove_first(linked_list_t * list);

// given a function that tests the values in the linked list, the first element that
// satisfies that function is removed.
// returns the new length of the linked list if successful, -1 otherwise
int
linked_list_remove_search(linked_list_t * list, int cond(void *));

// returns a pointer to the `n`th value in the linked list.
// returns `NULL` if unsuccessful
void *
linked_list_get_n(linked_list_t * list, int n);

// returns a pointer to the first value in a linked list.
// `NULL` if empty
void *
linked_list_get_first(linked_list_t * list);

// runs f on alinked_list values of list
void
linked_list_map(linked_list_t * list, gen_fun_t f);

// goes through all the values of a linked list and calls `list->val_printer` on them
void
linked_list_print(linked_list_t list);

// a generic taredown function for values that don't need anything done
void
linked_list_no_teardown(void * n);


#endif  /* LINKED_LIST_H */
