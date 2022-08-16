#include "os_test_asserts.h"
#include "os_test_runner.h"

#include "fs/dir.h"
#include "fs/fs.h"
#include "fs/inodes.h"
#include "fs/log.h"
#include "fs/pathname.h"

#include "kernel/memory.h"
#include "kernel/x86.h"

#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void validname_test();
static void skipelem_test();
static void path_parent_test();
static void path_lookup_test();

void pathname_test() {
	test_task_t tasks[] = {
		CREATE_TEST_TASK(validname_test), 
		CREATE_TEST_TASK(skipelem_test),
		CREATE_TEST_TASK(path_parent_test),
		CREATE_TEST_TASK(path_lookup_test),
	};
	os_test_run(tasks, sizeof(tasks) / sizeof(test_task_t));
}

static void validname_test() {
	static char *valid_names[] = {
		"hello", "a.txt", "a.out", "a  .out", "abcdefghijklmn", "      ", "a..",
		"abc?",  "a(b)",  "b",	 "0",		  "a.aa",			"test()",
	};
	static char *invalid_names[] = {
		"+ab", "+", "", "-=", "\babc", "abc\b", "hshe\t\n",
	};
	for (uint i = 0; i < sizeof(valid_names) / sizeof(*valid_names); i++) {
		assert_true(path_valid_name(valid_names[i]));
	}
	for (uint i = 0; i < sizeof(invalid_names) / sizeof(*invalid_names); i++) {
		assert_false(path_valid_name(invalid_names[i]));
	}
}

static void skipelem_test() {

	struct skipelem_tc {
		char *input;               // Input path
		char *expected_path[10];   // Expected path elements.
		int expected_off[10];      // Expected path offset.
		int nelement;              // Number of elements.
	} testcases[] = {{"/abc", {"abc"}, {4}, 1},
					 {"abc", {"abc"}, {3}, 1},
					 {"abc/", {"abc"}, {3}, 1},
					 {"", {}, {}, 0},
					 {"/", {}, {}, 0},
					 {"///", {}, {}, 0},
					 {"abc/./b", {"abc", ".", "b"}, {3, 5, 7}, 3},
					 {"//abc//\n../b/", {"abc", "\n..", "b"}, {5, 10, 12}, 3},
					 {"///abc//../  d//efg../",
					  {"abc", "..", "  d", "efg.."},
					  {6, 10, 14, 21},
					  4}};
	
	for (uint i = 0; i < sizeof testcases / sizeof *testcases; i++) {
		struct skipelem_tc *tc = &testcases[i];
		char *s = tc->input;
		char name[DIRENT_NAME_LENGTH] = {0};
		int sidx = 0;
		while ((s = path_skipelem(s, name)) != NULL) {
			assert_str_equal(tc->expected_path[sidx], name);
			assert_str_equal(tc->input + tc->expected_off[sidx], s);
			sidx++;
		}
		assert_int_equal(sidx, tc->nelement);
	}
}

static void path_parent_test() {
	struct {
		char *input;
		char *expected_parent;
		char *expected_name;
	} pp_test_cases[] = {{"ab/cd/efg", "ab/cd/", "efg"},
						 {"efg", "", "efg"},
						 {"/efg", "/", "efg"},
						 {"efg/", "", "efg"},
						 {"/", "", ""}};

	char parent[80], name[DIRENT_NAME_LENGTH];
	for (uint i = 0; i < sizeof(pp_test_cases) / sizeof(*pp_test_cases); i++) {
		memset(parent, 0, sizeof(parent));
		memset(name, 0, sizeof(name));
		path_parent(pp_test_cases[i].input, parent, name);
		assert_str_equal(pp_test_cases[i].expected_parent, parent);
		assert_str_equal(pp_test_cases[i].expected_name, name);
	}
}



struct path_record {
	char *path;
	struct inode *path_elements[10];
	int length;
	enum inode_type type;
};

// Help functions
static void mkf(struct path_record *r);
static void rmf(struct path_record *r);

static void path_lookup_test() {

	struct path_lookup_tc {
		char *mk_path;      // Create the file before test.
		char *inputs[10];   // Input paths for path_lookup/paht_lookup_parent.
		int found_elemidx;  // Index of (path_record->ips) or -1 if output is NULL.
		bool nameiparent;   // Lookup parent of input path?
		char *name;         // Name of the final path element(nameiparent is true).
	} testcases[] = {
		{"tmp/abc/efg", {"/tmp/abc/efg", "/tmp//abc///efg//", 0}, 2, false, 0},
		{"tmp/e", {"/tmp/s", "/tmp//a  b", "///tmp//e ", 0}, -1, false, 0},
		{"tmp/a", {"/tmp//abc", "/tmp/abc", "///tmp//abc", 0}, 0, true, "abc"},
		{"tmp/foo/bar",
		 {"/tmp/foo/bar", "/tmp/../tmp/foo/bar", 0},
		 2,
		 false,
		 0},
		{"tmp/foo/bar",
		 {"/tmp/foo/bar/foo", "/tmp/.././/../foo/./bar//",
		  "/tmp/./foo/bar/../foo", "/tmp/....../foo/bar", 0},
		 -1,
		 false,
		 0},
	};
	
	struct log *log = get_current_disk()->log;
	for (uint i = 0; i < sizeof testcases / sizeof *testcases; i++) {
		struct path_lookup_tc *tc = &testcases[i];
		char **input = tc->inputs;
		char name[DIRENT_NAME_LENGTH];
		struct inode *ip;
		struct path_record r;
		r.path = tc->mk_path;
		r.type = INODE_FILE;
		log_begin_op(log);
		mkf(&r);
		log_end_op(log);
		while (*input != 0) {
			log_begin_op(log);
			if (tc->nameiparent) {
				ip = path_lookup_parent(get_current_disk(), *input, name);
				assert_str_equal(tc->name, name);
			} else {
				ip = path_lookup(get_current_disk(), *input);
			}
			if (tc->found_elemidx == -1) {
				assert_ptr_equal(NULL, ip);
			} else {
				assert_ptr_not_equal(NULL, ip);
				assert_int_equal(r.path_elements[tc->found_elemidx]->inum,
								 ip->inum);
			}
			input++;
			log_end_op(log);
		}
		log_begin_op(log);
		rmf(&r);
		log_end_op(log);
	}
}

static void mkf(struct path_record *r) {
	char name[DIRENT_NAME_LENGTH];
	char *p = r->path;
	struct disk *disk = get_current_disk();
	struct inode *root = path_lookup(disk, "/");
	
	struct inode *ip = root;
	struct dirent dirent;
	int i = 0;
	while ((p = path_skipelem(p, name)) != NULL) {
		inode_lock(ip);
		assert_int_equal(INODE_DIRECTORY, ip->disk_inode.type);

		while (*p == '/') {
			p++;
		}
		struct inode *next = dir_lookup(ip, name, NULL);
		if (next == NULL) {
			enum inode_type typ = *p == 0 ? r->type : INODE_DIRECTORY;
			next = inode_alloc(disk, typ);
			assert_ptr_not_equal(NULL, next);
			inode_lock(next);
			strcpy(dirent.name, name);
			dirent.inum = next->inum;
			assert_true(dir_link(ip, &dirent) >= 0);
			if (typ == INODE_DIRECTORY) {
				dir_make(ip, next);
			}
			inode_unlock(next);
		}
		
		inode_dup(next);
		r->path_elements[i++] = next;
		inode_unlock(ip);
		ip = next;
	}
	r->length = i;
}

static void rmf(struct path_record *r) {
	struct dirent dirent;
	struct disk *disk = get_current_disk();
	struct inode *root = path_lookup(disk, "/");
	uint32_t off = 0;
	while (inode_read(root, &dirent, off, sizeof dirent) >= 0) {
		if (dirent.inum == r->path_elements[0]->inum) {
			dirent.inum = 0;
			memset(dirent.name, 0, DIRENT_NAME_LENGTH);
			inode_write(root, &dirent, off, sizeof dirent);
			break;
		}
		off += sizeof dirent;
	}
	for (int i = r->length - 1; i >= 0; i--) {
		r->path_elements[i]->ref = 1;
		r->path_elements[i]->disk_inode.nlink = 0;
		inode_put(r->path_elements[i]);
	}
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
