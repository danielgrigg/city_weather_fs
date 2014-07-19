//  Copyright (c) 2014 Daniel Grigg. All rights reserved.
//

#include "cityfs.hpp"
#include "cityfs_util.hpp"
#include "cityfs_weather.hpp"

namespace cityfs {

using namespace std;
using namespace cityfs::util;


string country_to_real_path(
      const CountryCodeMap& index,
      const string& country_code) {
    auto iter = index.code_to_country.find(country_code);
    if (iter != index.code_to_country.end()) {
      return iter->second;  
    }
    return country_code;
  }

string real_path_to_country(
      const CountryCodeMap& index,
      const string& country) {

    auto iter = index.country_to_code.find(country);
    if (iter != index.code_to_country.end()) {
      return iter->second;  
    }
    return country;
  }

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

// Check if a real-path maps to a virtual cityfs path.
bool virtual_path_exists(
    const CountryMap& country_map,
    const CountryCodeMap& country_code_map, 
    const string& real_path) {

  auto components = split(real_path.substr(1), '/');
  if (components.size() > 0) {
    
    // Files look like /Australia/Brisbane.txt, /Australia/Sydney.txt, ...
    auto code = real_path_to_country(country_code_map, components[0]);
    auto country_iter = country_map.find(code);
    if (country_iter != country_map.end()) {
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


// Get the content for a file.
// For cityfs, the real-path will always be of the form,
// /$country/$city.txt
tuple<string, Result> content_for_path(
    const CountryMap& country_map,
    const CountryCodeMap& country_code_map,
    const string& real_path, 
    bool get_weather) {

  auto components = split(real_path.substr(1), '/');
  auto result = Result::cityfs_unknown;
  if (components.size() > 0) {
    
    // Files look like /Australia/Brisbane.txt
    auto country_name = components[0];

    // The virtual fs works with country codes.
    auto country_code = real_path_to_country(
        country_code_map,
        country_name);

    auto country_iter = country_map.find(country_code);
    if (country_iter != country_map.end()) {
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

}
