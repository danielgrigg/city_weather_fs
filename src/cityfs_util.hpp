#ifndef CITYFS_UTIL_HPP
#define CITYFS_UTIL_HPP

#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

namespace cityfs { namespace util {

  template <typename T>
    void index_map(std::unordered_map<std::string, T>& items, 
        std::vector<std::string>& index) {

      index.clear();
      index.reserve(items.size());
      for (auto item : items) {
        index.push_back(item.first);
      }
    }

  inline std::vector<std::string> split(
      const std::string& s, 
      char delimiter) {

    std::stringstream iss(s);
    std::string line;
    std::vector<std::string> tokens;
    tokens.reserve(8);
    while (getline(iss, line, delimiter)) {
      tokens.push_back(line);
    }
    return tokens;
  }

  inline std::string trim_extension(const std::string& path) {

    auto idx = path.find_last_of("."); 
    if (idx == std::string::npos) {
      return path;
    }
    return path.substr(0, idx); 
  }

}
}

#endif
