#include <stdlib.h>
#include <stdio.h>
#include "linked_list.h"
#include "job.h"

// Creates and returns a new list
// If compare is NULL, list_insert just inserts at the head
list_t * list_create(compare_fn compare)
{
    list_t *list = (list_t *)malloc(sizeof(list_t));
    if (list == NULL)
    {
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->compare = compare;
    return list;
}

// Destroys a list
void list_destroy(list_t *list)
{
    list_node_t *current = list->head;
    while (current != NULL)
    {
        list_node_t *next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

// Returns head of the list
list_node_t *list_head(list_t *list)
{
    return list->head;
}

// Returns tail of the list
list_node_t *list_tail(list_t *list)
{
    return list->tail;
}

// Returns next element in the list
list_node_t *list_next(list_node_t *node)
{
    return node->next;
}

// Returns prev element in the list
list_node_t *list_prev(list_node_t *node)
{
    return node->prev;
}

// Returns end of the list marker
list_node_t *list_end(list_t *list)
{
    return NULL;
}

// Returns data in the given list node
void *list_data(list_node_t *node)
{
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t *list)
{
    return list->count;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t *list_find(list_t *list, void *data)
{
    list_node_t *current = list->head;
    while (current != NULL)
    {
        if (list->compare != NULL ? list->compare(current->data, data) == 0: current->data == data)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Inserts a new node in the list with the given data
// Returns new node inserted
list_node_t *list_insert(list_t *list, void *data)
{
    list_node_t *new_node = (list_node_t *)malloc(sizeof(list_node_t));
    if (new_node == NULL)
    {
        return NULL;
    }
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = NULL;

    if (list->head == NULL)
    {
        list->head = new_node;
        list->tail = new_node;
    }
    else if (list->compare == NULL)
    {
        new_node->next = list->head;
        list->head->prev = new_node;
        list->head = new_node;
    }
    else
    {
        list_node_t *current = list->head;
        while (current != NULL && list->compare(data, current->data) > 0)
        {
            current = current->next;
        }
        if (current == NULL)
        {
            list->tail->next = new_node;
            new_node->prev = list->tail;
            list->tail = new_node;
        }
        else if (current->prev == NULL)
        {
            new_node->next = list->head;
            list->head->prev = new_node;
            list->head = new_node;
        }
        else
        {
            new_node->next = current;
            new_node->prev = current->prev;
            current->prev->next = new_node;
            current->prev = new_node;
        }
    }

    list->count++;

    return new_node;
}

// Removes a node from the list and frees the node resources
void list_remove(list_t *list, list_node_t *node)
{
    if (list->head == NULL || node == NULL)
    {
        return;
    }

    if (node->prev == NULL)
    {
        list->head = node->next;
    }
    else
    {
        node->prev->next = node->next;
    }

    if (node->next == NULL)
    {
        list->tail = node->prev;
    }
    else
    {
        node->next->prev = node->prev;
    }

    free(node);
    list->count--;
}