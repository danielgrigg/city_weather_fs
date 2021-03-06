#ifndef CITYFS_HPP
#define CITYFS_HPP

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <fstream>
#include <sstream>
#include <tuple>
#include "country_codes.hpp"
#include "cityfs_util.hpp"

namespace cityfs {

  enum class PathMatch {
    cityfs_unknown,
    cityfs_city,
    cityfs_country
  };

  struct City {
    std::string name;
    std::string latitude;
    std::string longitude;
    std::string population;
    std::string timezone;
  };

  inline std::string city_to_path(const std::string& name) { 
    return name + ".txt"; 
  }

  inline std::string path_to_city(const std::string& path) {
    return util::trim_extension(path);
  }

  inline std::ostream& operator<<(std::ostream& os, const City& rhs) {
    os << rhs.name << ", "
      << rhs.latitude << ", "
      << rhs.longitude << ", "
      << rhs.population << ", "
      << rhs.timezone;
    return os;
  }

  struct Country {
    std::string name;
    std::vector<std::string> city_names;
    std::unordered_map<std::string, City> city_map;
  };

  inline std::ostream& operator<<(std::ostream& os, const Country& c) {
    os << c.name << ":" << std::endl;
    for (auto city_pair : c.city_map) {
      os << city_pair.second << std::endl;
    }
    return os;
  }

  typedef std::unordered_map<std::string, Country> CountryMap;

  std::string country_to_path(
      const CountryCodeMap& index,
      const std::string& country_code);

  std::string path_to_country(
      const CountryCodeMap& index,
      const std::string& country);

  bool parse_cities(
      const std::string& path, 
      std::unordered_map<std::string, Country>& countries);


  // Check if a real-path maps to a virtual cityfs path.
  bool virtual_path_exists(
      const CountryMap& country_map,
      const CountryCodeMap& country_code_map, 
      const std::string& path);

  // Get the content for a file.
  // For cityfs, the real-path will always be of the form,
  // /$country/$city.txt
  std::tuple<std::string, PathMatch> content_for_path(
      const CountryMap& country_map,
      const CountryCodeMap& country_code_map,
      const std::string& path, 
      bool get_weather=false);
}

#endif
