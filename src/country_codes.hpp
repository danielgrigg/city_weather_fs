#include <unordered_map>
#include <string>

typedef std::unordered_map<std::string, std::string> StringMap;

struct CountryCodeMap {
  StringMap code_to_country;
  StringMap country_to_code;
};

CountryCodeMap all_country_codes();

