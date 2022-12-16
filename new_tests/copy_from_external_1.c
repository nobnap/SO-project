#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char const initial_string[] = "this is the inicial file.";
    char *str_ext_file = "this is a string.";
    char *path_copied_file = "/f1";
    char *path_src = "new_tests/small_file.txt";
    char buffer[40];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r, w;

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    w = tfs_write(f, initial_string, sizeof(initial_string));
    assert(w == sizeof(initial_string));

    tfs_close(f);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer)-1);
    assert(r == sizeof(initial_string));
    assert(!memcmp(buffer, initial_string, strlen(initial_string)));

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    printf("Successful test.\n");

    return 0;
}
