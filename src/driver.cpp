//  Copyright (c) 2014 Daniel Grigg. All rights reserved.
//
//
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include "cityfs.hpp"
#include "cityfs_util.hpp"
#include "cityfs_weather.hpp"

using namespace std;
using namespace cityfs;
using namespace cityfs::util;


static unordered_map<string, string> g_open_cache;
static vector<string> g_country_codes;
static CountryCodeMap g_country_code_map;

// Cache contents on file open. The cache works on 'real-paths'.
static CountryMap g_country_map;

/// handle getting file attributes
static int cityfs_getattr(const char *real_path,
                          struct stat *stbuf) {
  cerr << "GETATTR " << real_path << endl;
  memset(stbuf, 0, sizeof(struct stat));
  
  // The root directory of our file system.
  if (string(real_path) == "/") {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }

  string content;
  Result result;

  // don't get the weather for statting the file
  // this also implies content sizes will vary on read.
  tie(content, result) = content_for_path(
      g_country_map,
      g_country_code_map,
      real_path, 
      false);
  if (result == Result::cityfs_directory ) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }

  if (result == Result::cityfs_file) {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = content.length();
    return 0;
  }
  
  return 0;
}

// handle opening files
static int cityfs_open(const char *real_path, struct fuse_file_info *fi)
{
  cerr << "OPEN " << real_path << endl;

  string path_str = real_path;
  if (!virtual_path_exists( g_country_map, g_country_code_map, path_str)) { 
    return -ENOENT;
  }

  std::string content;
  Result result;
  tie(content, result) = content_for_path(
      g_country_map,
      g_country_code_map,
      path_str, 
      true);
  if (result == Result::cityfs_file) {
    g_open_cache[path_str] = content;
  }
  
  /* Only reading allowed. */
  if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
  
  return 0;
}


// handle reading a directory
static int cityfs_readdir(const char *real_path,
                          void *buf,
                          fuse_fill_dir_t filler,
                          off_t offset,
                          fuse_file_info *fi)  {
  cerr << "READDIR " << real_path << endl;
  string virtual_path = real_path;
  
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  if (virtual_path == "/") {
    for (auto code : g_country_codes) {
      auto country = country_to_real_path(g_country_code_map, code);
      filler(buf, country.c_str(), NULL, 0);
    }
    return 0;
  }
  
  auto virtual_path_noroot = virtual_path.substr(1);
  auto components = split(virtual_path_noroot, '/');
  if (components.size() > 0) {
    auto code = real_path_to_country(
        g_country_code_map,
        components[0]);
    for (auto c : g_country_map[code].city_names) {
      filler(buf, city_to_real_path(c).c_str(), NULL, 0);
    }
    return 0;
  }
  return -ENOENT;
}

// handle reading a file
static int cityfs_read(const char *real_path,
                       char *buf,
                       size_t size,
                       off_t offset,
                       fuse_file_info *fi)  {
  cerr << "READ " << real_path << "(" << size << ")" << endl;
  auto virtual_path = real_path_to_city(real_path); 

  auto city_iter = g_open_cache.find(real_path);
  if (city_iter == g_open_cache.end()) {
    cerr << "ERROR Reading open_cache for real_path " << real_path << endl;
    return 0;
  }
  auto content = city_iter->second;
  cout << "READ " << content.size() << "B, " << "<" << content << ">\n";
  
  if (offset >= content.size())  return 0;
  
  // trim to available content
  auto actual_size = offset + size > content.size() ? content.size() - offset :  size;
  memcpy(buf, content.c_str() + offset, actual_size);
  return static_cast<int>(actual_size);
}


static struct fuse_operations cityfs_filesystem_operations = {
  .getattr = cityfs_getattr, /* To provide size, permissions, etc. */
  .open    = cityfs_open,    /* To enforce read-only access.       */
  .read    = cityfs_read,    /* To provide file content.           */
  .readdir = cityfs_readdir, /* To provide directory listing.      */
};


int main(int argc, const char * argv[]) {

  if (argc < 3) {
    cout << "Usage: " << argv[0] << " [city-file] [mount-point] \n\n";
    cout << "city-file must be a csv of (country-code,city,lat,lng,elevation,region)\n\n";
    cout << "  For example:\n";
    cout << "    $ cityfs cities15k.csv ~/cities \n\n";
    cout << "  Where cities15k.csv looks like:\n";
    cout << "    AU,Gold Coast,-28.00029,153.43088,591473,Australia/Brisbane\n";
    cout << "    AU,Gladstone,-23.84761,151.25635,30489,Australia/Brisbane\n";
    cout << "    AU,Geelong,-38.14711,144.36069,226034,Australia/Melbourne\n";
    return 1;
  } 
  auto city_file = argv[1];
  auto mount_point = argv[2];

  if (!parse_cities(city_file, g_country_map)) return 1;

  g_country_code_map = all_country_codes();

  index_map<Country>(g_country_map, g_country_codes);
  
  for (auto& country_pair : g_country_map) {
    index_map<City>(country_pair.second.city_map, 
        country_pair.second.city_names);
  }
 
  weather_init(); 
  
  cout << "Mounting cityfs..." << endl;

  // Add flags to argv_fused for debugging, eg, {"-d", "-f"};
  const char* argv_fused[] = {argv[0], argv[2]};
  int argc_fused = sizeof(argv_fused) / sizeof(char*);

  return fuse_main(argc_fused, 
      (char**)argv_fused, 
      &cityfs_filesystem_operations, 
      NULL);
}

