
#include "cityfs_weather.hpp"
#include "http_kit.h"
#include "rapidjson/document.h"
#include <sstream>

using namespace rapidjson;
using namespace std;
using namespace cov;

namespace cityfs {

  bool read_json(Document& doc, const std::string& input) {
    return !doc.Parse<0>(input.c_str()).HasParseError();
  }

  void weather_init() {
    cov::http_global_init();
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


}