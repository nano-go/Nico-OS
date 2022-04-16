#include "os_test_asserts.h"
#include "os_test_runner.h"
#include "kernel/list.h"

struct int_element {
	uint32_t int_val1;
	struct list_node node_tag;
};

void stack_ops_test();
void queue_ops_test();
void find_test();
void clear_test();

void list_test() {
	test_task_t tasks[] = { 
		CREATE_TEST_TASK(stack_ops_test), 
		CREATE_TEST_TASK(queue_ops_test),
		CREATE_TEST_TASK(find_test), 
		CREATE_TEST_TASK(clear_test),
	};
	os_test_run(tasks, sizeof(tasks) / sizeof(test_task_t));
}

void stack_ops_test() {
	struct list list;
	list_init(&list);

	assert_true(list_empty(&list));
	
	struct int_element elements[10];
	for (int i = 0; i < 10; i++) {
		elements[i].int_val1 = i;
		assert_int_equal(i, list_size(&list));
		list_push(&list, &elements[i].node_tag);
	}
	
	assert_int_equal(10, list_size(&list));
	assert_false(list_empty(&list));

	struct int_element *e;
	struct list_node *n;
	int i = 0;
	
	LIST_FOREACH(e, &list, struct int_element, node_tag) {
		assert_int_equal(i, (int)e->int_val1);
		i++;
	}

	for (i = 0; i < 10; i ++) {
		n = list_get(&list, i);
		assert_true(n != NULL);
		e = NODE_AS(struct int_element, n, node_tag);
		assert_int_equal(i, e->int_val1);
	}
	
	for (i = 9; i >= 0; i--) {
		e = NODE_AS(struct int_element, list_pop(&list), node_tag);
		assert_int_equal(i, e->int_val1);
		assert_int_equal(i, list_size(&list));
	}

	assert_true(list_empty(&list));
}


void queue_ops_test() {
	struct list list;
	list_init(&list);
	
	struct int_element elements[10];
	for (int i = 0; i < 10; i++) {
		elements[i].int_val1 = i;
		assert_int_equal(i, list_size(&list));
		list_offer(&list, &elements[i].node_tag);
	}
	
	assert_int_equal(10, list_size(&list));
	assert_false(list_empty(&list));

	struct int_element *e;
	int i = 0;
	
	LIST_FOREACH(e, &list, struct int_element, node_tag) {
		assert_int_equal(10 - i - 1, (int)e->int_val1);
		i++;
	}
	
	for (i = 0; i < 10; i ++) {
		e = NODE_AS(struct int_element, list_poll(&list), node_tag);
		assert_int_equal(i, e->int_val1);
		assert_int_equal(10 - i - 1, list_size(&list));
	}
	
	assert_true(list_empty(&list));
}

void find_test() {
	struct list list;
	list_init(&list);

	struct int_element elements[10];
	for (int i = 0; i < 5; i++) {
		assert_int_equal(i, list_size(&list));
		list_push(&list, &elements[i].node_tag);
		assert_true(list_find(&list, &elements[i].node_tag));
	}

	for (int i = 5; i < 10; i++) {
		assert_false(list_find(&list, &elements[i].node_tag));
	}

	assert_int_equal(5, list_size(&list));
}

void clear_test() {
	struct list list;
	list_init(&list);

	struct int_element elements[10];
	for (int i = 0; i < 10; i++) {
		list_push(&list, &elements[i].node_tag);
	}

	assert_int_equal(10, list_size(&list));
	list_clear(&list);
	assert_int_equal(0, list_size(&list));

	struct int_element *e;
	LIST_FOREACH(e, &list, struct int_element, node_tag) {
		assert_false("Unreachable!!!");
	}
}