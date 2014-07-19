#ifndef CITYFS_UTILITY_HPP
#define CITYFS_UTILITY_HPP

namespace cityfs_util {

template <typename T>
void index_map(unordered_map<string, T>& items, vector<string>& index) {
  index.clear();
  index.reserve(items.size());
  for (auto item : items) {
    index.push_back(item.first);
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

string trim_extension(const string& path) {
  auto idx = path.find_last_of("."); 
  if (idx == string::npos) {
    return path;
  }
  return path.substr(0, idx); 
}


}

#endif
