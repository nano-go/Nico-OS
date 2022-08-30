#ifndef _KERNEL_LIST_H
#define _KERNEL_LIST_H

#include "kernel/debug.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */


/**
 * This converts a "struct list_node*" ptr into the specified type ptr.
 *
 * Usage:
 *    struct some_s {
 *        ...;
 *	    struct list_node *node_member;
 *	    ...;
 *	};
 *    struct some_s *element =
 *	    NODE_AS(struct some_s, list_poll(list), node_member);
 */
#define NODE_AS(type, node_ptr, member) (type *) ((uint8_t *) (node_ptr) -offsetof(type, member))

#define LIST_FIRST(list) ((list)->head.next)
#define LIST_LAST(list)  ((list)->tail.prev)

#define LIST_FOREACH(item, list, type, member)                                                     \
    for (item = NODE_AS(type, (list)->head.next, member); &item->member != &((list)->tail);        \
         item = NODE_AS(type, item->member.next, member))

struct list_node {
    struct list_node *prev;
    struct list_node *next;
};

struct list {
    struct list_node head;
    struct list_node tail;
};

static inline void list_init(struct list *l) {
    l->head.next = &l->tail;
    l->tail.prev = &l->head;
}

static inline void list_linked_before(struct list_node *n, struct list_node *new_node) {
    new_node->next = n;
    new_node->prev = n->prev;
    n->prev->next = new_node;
    n->prev = new_node;
}

static inline void list_linked_after(struct list_node *n, struct list_node *new_node) {
    new_node->prev = n;
    new_node->next = n->next;
    n->next->prev = new_node;
    n->next = new_node;
}

static inline void list_unlinked(struct list_node *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
}

static inline bool list_empty(struct list *l) {
    return l->head.next == &l->tail;
}

static inline void list_offer(struct list *l, struct list_node *n) {
    list_linked_after(&l->head, n);
}

static inline struct list_node *list_poll(struct list *l) {
    ASSERT(!list_empty(l));
    struct list_node *n = l->tail.prev;
    list_unlinked(n);
    return n;
}

static inline void list_push(struct list *l, struct list_node *n) {
    list_linked_before(&l->tail, n);
}

static inline struct list_node *list_pop(struct list *l) {
    ASSERT(!list_empty(l));
    struct list_node *n = l->tail.prev;
    list_unlinked(n);
    return n;
}

static inline void list_clear(struct list *l) {
    l->head.next = &l->tail;
    l->tail.prev = &l->head;
}

static inline bool list_find(struct list *l, struct list_node *target) {
    struct list_node *n = l->head.next;
    const struct list_node *end = &l->tail;
    while (n != end) {
        if (n == target) {
            return true;
        }
        n = n->next;
    }
    return false;
}

static inline uint32_t list_size(struct list *l) {
    struct list_node *n = l->head.next;
    const struct list_node *end = &l->tail;
    uint32_t size = 0;
    while (n != end) {
        size++;
        n = n->next;
    }
    return size;
}

static inline struct list_node *list_get(struct list *l, uint32_t idx) {
    ASSERT(idx < list_size(l));
    struct list_node *n = l->head.next;
    const struct list_node *end = &l->tail;
    for (uint32_t i = 0; i != idx && n != end; i++, n = n->next)
        /* Nothing */;
    if (n == end) {
        return NULL;
    }
    return n;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* __KERNEL_LIST_H */