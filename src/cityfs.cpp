//
//  main.cpp
//  cityfs
//
//  Created by Daniel Grigg on 13/01/2014.
//  Copyright (c) 2014 Daniel Grigg. All rights reserved.
//


#define FUSE_USE_VERSION 26

#include <iostream>
#include <fuse.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <fstream>
#include <sstream>
#include <tuple>
#include "http_kit.h"
#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

template <typename T>
void index_map(unordered_map<string, T>& xs, vector<string>& index) {
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
  unordered_map<string, City> city_map;
};

ostream& operator<<(ostream& os, const Country& c) {
  os << c.name << ":" << endl;
  for (auto city_pair : c.city_map) {
    os << city_pair.second << endl;
  }
  return os;
}

static unordered_map<string, Country> g_country_map;
static vector<string> g_country_codes;

// Cache contents on file open
static unordered_map<string, string> g_open_cache;

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

bool read_json(Document& doc, const std::string& input) {
  return !doc.Parse<0>(input.c_str()).HasParseError();
}

string weather_content(const string& city) {
  /*
   Result<Key, BasicStatus> parse_key(const string& response) {
   Document document;
   if (!read_json(document, response)) {
   return { {}, BasicStatus::error_invalid_json };
   }
   printf("response:\n %s", response.c_str());
   
   if (!is_key_response(document))  {
   printf("\ninvalid response\n");
   return { {}, BasicStatus::error_invalid_response };
   }
   
   const Value& keys = document["keys"];
   auto& k = keys[SizeType(0)];
   */
  
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
  std::string content;
  Result result;
  tie(content, result) = content_for_path(path, true);
  if (result == Result::cityfs_file) {
    g_open_cache[path] = content;
  }
  
  
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
    auto country = components[0];
    //    cout << "[DEBUG] Reading country '" << country << "'" << endl;
    
    for (auto c : g_country_map[country].city_names) {
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

  auto city_iter = g_open_cache.find(path);
  if (city_iter == g_open_cache.end()) {
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

bool parse_cities(const string& path, unordered_map<string, Country>& countries) {
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
    
    next.name += ".txt";
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
  
  cov::http_global_init();
  
  return fuse_main(argc, (char**)argv, &cityfs_filesystem_operations, NULL);
}


