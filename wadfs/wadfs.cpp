#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "../libWad/Wad.h"

using namespace std;

static int wadfs_getattr(const char* path, struct stat* stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (((Wad*)fuse_get_context()->private_data)->isDirectory(path)) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        return 0;
    } else if (((Wad*)fuse_get_context()->private_data)->isContent(path)) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = ((Wad*)fuse_get_context()->private_data)->getSize(path);
        return 0;
    }

    return -ENOENT;
}

static int wadfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    std::vector<std::string> entries;
    int count = ((Wad*)fuse_get_context()->private_data)->getDirectory(path, &entries);
    if (count == -1) return -ENOENT;

    for (const auto& entry : entries) {
        filler(buf, entry.c_str(), NULL, 0);
    }

    return 0;
}

static int wadfs_open(const char* path, struct fuse_file_info* fi) {
    if (!((Wad*)fuse_get_context()->private_data)->isContent(path)) {
        return -ENOENT;
    }
    return 0;
}

static int wadfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    (void) fi;
    return ((Wad*)fuse_get_context()->private_data)->getContents(path, buf, size, offset);
}

static int wadfs_mkdir(const char* path, mode_t mode) {
    (void) mode;
    ((Wad*)fuse_get_context()->private_data)->createDirectory(path);
    return 0;
}

static int wadfs_mknod(const char* path, mode_t mode, dev_t rdev) {
    (void) mode;
    (void) rdev;
    ((Wad*)fuse_get_context()->private_data)->createFile(path);
    return 0;
}

static int wadfs_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    (void) fi;
    return ((Wad*)fuse_get_context()->private_data)->writeToFile(path, buf, size, offset);
}

static struct fuse_operations operations = {
    .getattr = wadfs_getattr,
    .mknod = wadfs_mknod,
    .mkdir = wadfs_mkdir,
    .open = wadfs_open,
    .read = wadfs_read,
    .write = wadfs_write,
    .readdir = wadfs_readdir,
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Not enough arguments." << endl;
        exit(EXIT_SUCCESS);
    }

    string wadPath = argv[argc - 2];

    if (wadPath.at(0) != '/') {
        wadPath = string(get_current_dir_name()) + "/" + wadPath;
    }
    Wad* myWad = Wad::loadWad(wadPath);

    argv[argc - 2] = argv[argc - 1];
    argc--;

    return fuse_main(argc, argv, &operations, myWad);
}