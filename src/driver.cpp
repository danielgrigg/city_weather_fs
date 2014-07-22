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


static unordered_map<string, string> open_cache;
static vector<string> country_codes;
static CountryCodeMap country_code_map;

// Cache contents on file open. The cache works on 'real-paths'.
static CountryMap country_map;

/// handle getting file attributes
static int cityfs_getattr(const char *path, 
    struct stat *stbuf) {

  cerr << "GETATTR " << path << endl;
  memset(stbuf, 0, sizeof(struct stat));
  string content;
  PathMatch result;
  
  // Matched the root directory
  if (string(path) == "/") {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }

  // Query the path against our city-db.
  tie(content, result) = content_for_path(country_map, 
      country_code_map, path, false);

  // Matched a country, return a directory.
  if (result == PathMatch::cityfs_country ) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }

  // Matched a city
  if (result == PathMatch::cityfs_city) {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = content.length();
    return 0;
  }

  // Nothing matched
  return -ENOENT;
}

// handle opening files
static int cityfs_open(const char *cpath, 
    fuse_file_info *fi) {

  string path = cpath;
  cerr << "OPEN " << path << endl;

  if (!virtual_path_exists( country_map, country_code_map, path)) { 
    return -ENOENT;
  }

  std::string content;
  PathMatch result;
  tie(content, result) = content_for_path(
      country_map,
      country_code_map,
      path, 
      true);
  if (result == PathMatch::cityfs_city) {
    open_cache[path] = content;
  }
  
  if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
  return 0;
}


// handle reading a directory
static int cityfs_readdir(const char *cpath,
                          void *buf,
                          fuse_fill_dir_t filler,
                          off_t offset,
                          fuse_file_info *fi)  {
  string path = cpath;
  cerr << "READDIR " << path << endl;
  
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  if (path == "/") {
    for (auto code : country_codes) {
      auto country = country_to_path(country_code_map, code);
      filler(buf, country.c_str(), NULL, 0);
    }
    return 0;
  }
  
  auto relative = path.substr(1);
  auto components = split(relative, '/');

  // First directory is the country
  if (components.size() == 1) {
    auto code = path_to_country(country_code_map, components[0]);
    for (auto city : country_map[code].city_names) {
      filler(buf, city_to_path(city).c_str(), NULL, 0);
    }
    return 0;
  }
  return -ENOENT;
}

// handle reading a file
static int cityfs_read(const char *path,
                       char *buf,
                       size_t size,
                       off_t offset,
                       fuse_file_info *fi)  {
  cerr << "READ " << path << "(" << size << ")" << endl;
  auto virtual_path = path_to_city(path); 

  auto city_iter = open_cache.find(path);
  if (city_iter == open_cache.end()) {
    cerr << "ERROR Reading open_cache for path " << path << endl;
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

  if (!parse_cities(city_file, country_map)) return 1;

  country_code_map = all_country_codes();

  index_map<Country>(country_map, country_codes);
  
  for (auto& country_pair : country_map) {
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

