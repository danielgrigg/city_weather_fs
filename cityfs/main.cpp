//
//  main.cpp
//  cityfs
//
//  Created by Daniel Grigg on 13/01/2014.
//  Copyright (c) 2014 Daniel Grigg. All rights reserved.
//

#include <iostream>
#include <fuse.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <set>

using namespace std;
set<string> cities = { "Adelaide", "Brisbane", "Canberra", "Darwin", "Hobart", "Melbourne", "Perth", "Sydney" };

static int cityfs_getattr(const char *path,
                          struct stat *stbuf)
{
  memset(stbuf, 0, sizeof(struct stat));
  
  // The root directory of our file system.
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }
  bool matched = cities.count(path) > 0;
  if (matched) {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = sizeof(char) * (strlen(path) - 1);
    return 0;
  }
  return -ENOENT;
}

static int cityfs_open(const char *path, struct fuse_file_info *fi)
{
  //  if (strcmp(path, file_path) != 0) return -ENOENT;
  
  /* Only reading allowed. */
  if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
  
  return 0;
}



static int cityfs_readdir(const char *path,
                          void *buf,
                          fuse_fill_dir_t filler,
                          off_t offset,
                          fuse_file_info *fi)  {
  /* We only recognize the root directory. */
  if (strcmp(path, "/") != 0) return -ENOENT;
  
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  //filler(buf, file_path + 1, NULL, 0); /* The only file we have. */
  
  return 0;
}

static int cityfs_read(const char *path,
                       char *buf,
                       size_t size,
                       off_t offset,
                       fuse_file_info *fi)  {
  //    if (strcmp(path, file_path) != 0) return -ENOENT;
  
  /* Trying to read past the end of file. */
  //  if (offset >= file_size)  return 0;
  
  /* Trim the read to the file size. */
  //    if (offset + size > file_size) size = file_size - offset;
  
  //   memcpy(buf, file_content + offset, size); /* Provide the content. */
  
  return size;
}


static struct fuse_operations cityfs_filesystem_operations = {
  .getattr = cityfs_getattr, /* To provide size, permissions, etc. */
  .open    = cityfs_open,    /* To enforce read-only access.       */
  .read    = cityfs_read,    /* To provide file content.           */
  .readdir = cityfs_readdir, /* To provide directory listing.      */
};

int main(int argc, const char * argv[])
{
  cout << "Starting cityfs...";
  return fuse_main(argc, (char**)argv, &cityfs_filesystem_operations, NULL);
  
}

