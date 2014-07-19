//
//  main.cpp
//  cityfs
//
//  Created by Daniel Grigg on 13/01/2014.
//  Copyright (c) 2014 Daniel Grigg. All rights reserved.
//

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include "http_kit.h"
#include "rapidjson/document.h"
#include "cityfs_util.hpp"
#include "cityfs.hpp"

using namespace std;
using namespace rapidjson;
using namespace cityfs;
using namespace cityfs::util;



static CountryCodeMap g_country_code_map;
static unordered_map<string, Country> g_country_map;
static vector<string> g_country_codes;

// Cache contents on file open
static unordered_map<string, string> g_open_cache;

enum class Result {
  cityfs_unknown,
  cityfs_file,
  cityfs_directory
};

bool virtual_path_exists(const string& path) {
  auto components = split(path.substr(1), '/');
  if (components.size() > 0) {
    
    // Files look like /AU/Brisbane, /AU/Sydney, ...
    auto code = real_path_to_country(g_country_code_map, 
        components[0]);
    auto country_iter = g_country_map.find(code);
    if (country_iter != g_country_map.end()) {
      auto country = country_iter->second;
      
      // Reading directory
      if (components.size() == 1) {
        return true;
      }
      if (components.size() > 1) {
        auto city_name = real_path_to_city(components[1]);
        auto city_iter = country.city_map.find(city_name);
        if (city_iter != country.city_map.end()) {
          return true;
        }
      }
    }
  }
  return false;
}

bool read_json(Document& doc, const std::string& input) {
  return !doc.Parse<0>(input.c_str()).HasParseError();
}

string weather_content(const string& city) {
  auto uri = string("api.openweathermap.org/data/2.5/weather?q=") + city;
  string response;
  auto get_result = cov::http_get(uri , {{}}, response);
  Document doc;
  if (get_result != 0 || !read_json(doc, response)) {
    return "weather_unknown";
  }
  ostringstream oss;
  
  if (!(doc["weather"].IsArray() &&
      doc["weather"].Size() > 0 &&
      doc["weather"][SizeType(0)]["description"].IsString() &&
      doc["main"].IsObject() &&
      doc["main"]["temp"].IsDouble())) {
    return "weather_unknown";
  }
  
  auto description = doc["weather"][SizeType(0)]["description"].GetString();
  auto temperature = doc["main"]["temp"].GetDouble() - 273.15;
  oss << temperature << ", " << description;
  return oss.str();
}


tuple<string, Result> content_for_path(const string& path, bool get_weather=false) {
  auto components = split(path.substr(1), '/');
  auto result = Result::cityfs_unknown;
  if (components.size() > 0) {
    
    // Files look like /AU/Brisbane, /AU/Sydney, ...
    auto country_name = components[0];
    auto country_code = real_path_to_country(
        g_country_code_map,
        country_name);

    auto country_iter = g_country_map.find(country_code);
    if (country_iter != g_country_map.end()) {
      auto country = country_iter->second;
      
      if (components.size() == 1) {
        return make_tuple("", Result::cityfs_directory);
      }

      if (components.size() == 2) {
        auto city_name = real_path_to_city(components[1]);

        auto city_iter = country.city_map.find(city_name);
        if (city_iter != country.city_map.end()) {
          auto city = city_iter->second;
          ostringstream oss;
          oss << city.name << "," << city.latitude << "," << city.longitude;
          if (get_weather) {
            oss << "," << weather_content(city.name);
          } else {
            oss << "                                      ";
          }
          oss << "\n";
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
  if (string(path) == "/") {
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

  string path_str = path;
  if (!virtual_path_exists(path_str)) return -ENOENT;

  std::string content;
  Result result;
  tie(content, result) = content_for_path(path_str, true);
  if (result == Result::cityfs_file) {
    g_open_cache[path_str] = content;
  }
  
  /* Only reading allowed. */
  if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;
  
  return 0;
}

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

static int cityfs_read(const char *path,
                       char *buf,
                       size_t size,
                       off_t offset,
                       fuse_file_info *fi)  {
  cerr << "READ " << path << "(" << size << ")" << endl;
  auto virtual_path = real_path_to_city(path); 

  auto city_iter = g_open_cache.find(virtual_path);
  if (city_iter == g_open_cache.end()) {
    cerr << "ERROR Reading open_cache for virtual_path " << virtual_path << endl;
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

bool parse_cities(const string& path, 
    unordered_map<string, Country>& countries) {

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
  
  cov::http_global_init();
  
  cout << "Mounting cityfs..." << endl;

  const char* argv_fused[] = {argv[0], argv[2]};
  int argc_fused = sizeof(argv_fused) / sizeof(char*);

  return fuse_main(argc_fused, 
      (char**)argv_fused, 
      &cityfs_filesystem_operations, 
      NULL);
}

