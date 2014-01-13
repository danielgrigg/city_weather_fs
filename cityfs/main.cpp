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
set<string> cities = { "/Adelaide", "/Brisbane", "/Canberra",
  "/Darwin", "/Hobart", "/Melbourne", "/Perth", "/Sydney" };

unordered_map<string, const char*> city_contents = {
  { "Adelaide", "City of churches\n" },
  { "Brisbane", "Australia's New World City\n" },
  { "Canberra", "The Most Boring Town in Australia\n" },
  { "Darwin", "We have Crocodiles\n" },
  { "Hobart", "Welcome to the Island\n" },
  { "Melbourne", "So trendy\n" },
  { "Sydney", "We're cooler than Melbourne, really!\n" },
  { "Perth", "What's an Australia?\n" }};
  

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
    auto city_name = path + 1; // Ignore '/'
    auto content = city_contents[city_name];
    auto content_size = strlen(content);
    
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = content_size;
    return 0;
  }
  return -ENOENT;
}

static int cityfs_open(const char *path, struct fuse_file_info *fi)
{
  if (cities.count(path) == 0) return -ENOENT;
  
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
  
  for (auto city : cities) {
    filler(buf, city.c_str() + 1, NULL, 0);
  }
  
  return 0;
}

static int cityfs_read(const char *path,
                       char *buf,
                       size_t size,
                       off_t offset,
                       fuse_file_info *fi)  {
  if (cities.count(path) == 0) return -ENOENT;
  string city_name = path + 1;
  auto content = city_contents[city_name];
  auto content_size = strlen(content);
  
  /* Trying to read past the end of file. */
  if (offset >= content_size)  return 0;
  
  /* Trim the read to the file size. */
  auto actual_size = offset + size > content_size ? content_size - offset : size;
 
  /* Provide the content. */
  memcpy(buf, content + offset, actual_size);
  
  return static_cast<int>(actual_size);
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

