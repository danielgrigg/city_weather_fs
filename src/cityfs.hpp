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

namespace cityfs {

  struct City {
    std::string name;
    std::string latitude;
    std::string longitude;
    std::string population;
    std::string timezone;
  };

  inline std::string city_to_real_path(const std::string& name) { 
    return name + ".txt"; 
  }

  inline std::string real_path_to_city(const std::string& path) {
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

  std::ostream& operator<<(std::ostream& os, const Country& c) {
    os << c.name << ":" << std::endl;
    for (auto city_pair : c.city_map) {
      os << city_pair.second << std::endl;
    }
    return os;
  }

  inline std::string country_to_real_path(
      CountryCodeMap& index,
      const std::string& country_code) {
    auto iter = index.code_to_country.find(country_code);
    if (iter != index.code_to_country.end()) {
      return iter->second;  
    }
    return country_code;
  }

  inline std::string real_path_to_country(
      CountryCodeMap& index,
      const std::string& country) {

    auto iter = index.country_to_code.find(country);
    if (iter != index.code_to_country.end()) {
      return iter->second;  
    }
    return country;
  }
}

#endif
