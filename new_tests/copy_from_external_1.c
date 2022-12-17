#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    tfs_params params = tfs_default_params();
    size_t num = params.block_size;

    char text[] =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
        "eiusmod tempor incididunt ut labore et dolore magna aliqua. Nascetur "
        "ridiculus mus mauris vitae ultricies. Magna etiam tempor orci eu "
        "lobortis elementum. Leo integer malesuada nunc vel. Sed ullamcorper "
        "morbi tincidunt ornare massa eget egestas. Gravida neque convallis a "
        "cras semper auctor neque. Consectetur a erat nam at lectus urna. "
        "Quisque non tellus orci ac auctor augue. Quam id leo in vitae turpis. "
        "Sit amet facilisis magna etiam tempor. Quis enim lobortis scelerisque "
        "fermentum. In pellentesque massa placerat duis ultricies lacus. "
        "Feugiat vivamus at augue eget. Dui vivamus arcu felis bibendum ut "
        "tristique et egestas."
        "Risus feugiat in ante metus dictum at. Proin nibh nisl condimentum id "
        "venenatis a. Erat velit scelerisque in dictum non consectetur. "
        "Egestas maecenas pharetra convallis posuere morbi leo. Amet est "
        "placerat in egestas erat imperdiet sed euismod nisi. Orci porta non "
        "pulvinar neque laoreet suspendisse. Massa sapien faucibus et molestie "
        "ac feugiat sed lectus vestibulum. Vulputate dignissim suspendisse in "
        "est ante. Cursus in hac habitasse platea dictumst quisque sagittis "
        "purus sit. Ac tortor vitae purus faucibus.";
    char *path_copied_file = "/f1";
    char *path_src = "new_tests/big_file.txt";
    char buffer[sizeof(text)];

    assert(tfs_init(&params) != -1);

    int f;
    ssize_t r;

    // Attempts to copy file bigger than block size
    assert(tfs_copy_from_external_fs(path_src, path_copied_file) != -1);

    // File should exist
    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    // External file should only have been partially copied
    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r < sizeof(text));
    assert(r == num);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
