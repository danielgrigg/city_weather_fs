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
#include <fstream>
#include <sstream>
#include <map>
#include <tuple>
using namespace std;

template <typename T>
void index_map(map<string, T>& xs, vector<string>& index) {
  index.clear();
  index.reserve(xs.size());
  for (auto x : xs) {
    index.push_back(x.first);
  }
}

vector<string> split(const string& s, char delimiter) {
  istringstream iss(s);
  string line;
  vector<string> tokens;
  tokens.reserve(8);
  while (getline(iss, line, delimiter)) {
    tokens.push_back(line);
  }
  return tokens;
}

struct City {
  string name;
  string latitude;
  string longitude;
  string population;
  string timezone;
};

ostream& operator<<(ostream& os, const City& rhs) {
  os << rhs.name << ", "
  << rhs.latitude << ", "
  << rhs.longitude << ", "
  << rhs.population << ", "
  << rhs.timezone;
  return os;
}

struct Country {
  string name;
  vector<string> city_names;
  map<string, City> city_map;
};

ostream& operator<<(ostream& os, const Country& c) {
  os << c.name << ":" << endl;
  for (auto city_pair : c.city_map) {
    os << city_pair.second << endl;
  }
  return os;
}

static map<string, Country> g_country_map;
static vector<string> g_country_codes;

enum class Result {
  cityfs_unknown,
  cityfs_file,
  cityfs_directory
};

bool path_exists(const string& path) {
  auto components = split(path.substr(1), '/');
  if (components.size() > 0) {
    
    // Files look like /AU/Brisbane, /AU/Sydney, ...
    auto country_iter = g_country_map.find(components[0]);
    if (country_iter != g_country_map.end()) {
      auto country = country_iter->second;
      
      // Reading directory
      if (components.size() == 1) {
        return true;
      }
      if (components.size() > 1) {
        auto city_iter = country.city_map.find(components[1]);
        if (city_iter != country.city_map.end()) {
          return true;
        }
      }
    }
  }
  return false;
}

tuple<string, Result> content_for_path(const string& path) {
  auto components = split(path.substr(1), '/');
  auto result = Result::cityfs_unknown;
  if (components.size() > 0) {
    
    // Files look like /AU/Brisbane, /AU/Sydney, ...
    auto country_name = components[0];
    auto country_iter = g_country_map.find(country_name);
    if (country_iter != g_country_map.end()) {
      auto country = country_iter->second;
      
      // Reading directory
      if (components.size() == 1) {
        return make_tuple("", Result::cityfs_directory);
      } else if (components.size() == 2) {
        auto city_iter = country.city_map.find(components[1]);
        if (city_iter != country.city_map.end()) {
          auto city = city_iter->second;
          ostringstream oss;
          oss << city.name << "\n" << city.latitude << ", " << city.longitude << "\n";
          return make_tuple(oss.str(), Result::cityfs_file);
        }
      }
    }
  }
  return make_tuple("", result);
}

static int cityfs_getattr(const char *path,
                          struct stat *stbuf) {
  cerr << "GETATTR " << path << endl;
  memset(stbuf, 0, sizeof(struct stat));
  
  // The root directory of our file system.
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 3;
    return 0;
  }
  string content;
  Result result;
  tie(content, result) = content_for_path(path);
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
  
  //  return -ENOENT;
  return 0;
}

static int cityfs_open(const char *path, struct fuse_file_info *fi)
{
  cerr << "OPEN " << path << endl;
  
  if (!path_exists(path)) return -ENOENT;
  
  /* Only reading allowed. */
  if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
  
  return 0;
}

static int cityfs_readdir(const char *path,
                          void *buf,
                          fuse_fill_dir_t filler,
                          off_t offset,
                          fuse_file_info *fi)  {
  cerr << "READDIR " << path << endl;
  
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  
  if (strcmp(path, "/") == 0) {
    for (auto country : g_country_codes) {
      filler(buf, country.c_str(), NULL, 0);
    }
    return 0;
  }
  
  auto components = split(path + 1, '/');
  if (components.size() > 0) {
    for (auto c : components) cout << "component: " << c << endl;

    auto country = components[0];
    cout << "[DEBUG] Reading country '" << country << "'" << endl;

    for (auto c : g_country_map[country].city_names) {
      cout << "Filling city " << c << endl;
      filler(buf, c.c_str(), NULL, 0);
    }
    return 0;
  }
  return -ENOENT;
}

static int cityfs_read(const char *path,
                       char *buf,
                       size_t size,
                       off_t offset,
                       fuse_file_info *fi)  {
  cerr << "READ " << path << "(" << size << ")" << endl;
  string content;
  Result result;
  tie(content, result) = content_for_path(path);
  if (result == Result::cityfs_file) {
    if (offset >= content.size())  return 0;
    
    // trim to available content
    auto actual_size = offset + size > content.size() ?
    content.size() - offset :
    size;
    memcpy(buf, content.c_str() + offset, actual_size);
    return static_cast<int>(actual_size);
  }
  
  return 0;
}


static struct fuse_operations cityfs_filesystem_operations = {
  .getattr = cityfs_getattr, /* To provide size, permissions, etc. */
  .open    = cityfs_open,    /* To enforce read-only access.       */
  .read    = cityfs_read,    /* To provide file content.           */
  .readdir = cityfs_readdir, /* To provide directory listing.      */
};

bool parse_cities(const string& path, map<string, Country>& countries) {
  ifstream ifs;
  ifs.open(path);
  if (!ifs.is_open()) {
    cerr << "Error reading cities.csv" << endl;
    return false;
  }
  string line;
  
  while(getline(ifs, line)) {
    City next;
    string country_code;
    istringstream iss(line);
    
    // cout << "Reading line: " << line << endl;
    getline(iss, country_code, ',');
    getline(iss, next.name, ',');
    getline(iss, next.latitude, ',');
    getline(iss, next.longitude, ',');
    getline(iss, next.population, ',');
    getline(iss, next.timezone, ',');
    
    countries[country_code].city_map.insert(make_pair(next.name, next));
    countries[country_code].name = country_code;
  }
  ifs.close();
  return true;
}

int main(int argc, const char * argv[])
{
  cout << "Starting cityfs..." << endl;
  
  if (!parse_cities("cities15k.csv", g_country_map)) return 1;
  index_map<Country>(g_country_map, g_country_codes);
  
  for (auto& country_pair : g_country_map) {
    index_map<City>(country_pair.second.city_map, country_pair.second.city_names);
  }
  
  for (auto& country_pair : g_country_map) {
    for (auto c : country_pair.second.city_names) cout << "city: " << c << endl;
    //cout << country_pair.second << endl;
  }
  
  auto found = path_exists("/AU/Orange");
  
  return fuse_main(argc, (char**)argv, &cityfs_filesystem_operations, NULL);
  
}


