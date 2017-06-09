
#include "cityfs_weather.hpp"
#include "http_kit.hpp"
#include "rapidjson/document.h"
#include <sstream>

using namespace rapidjson;
using namespace std;
using namespace citynet;

namespace cityfs {

  // This is a public, rate-limited key so I can't guarantee it'll 
  // stay available.  You can obtain a free key here http://openweathermap.org/appid
  const std::string OpenWeatherMapKey = "4e23be0473684c5e1c69ebb3252f35a4";

  bool read_json(Document& doc, const std::string& input) {
    return !doc.Parse<0>(input.c_str()).HasParseError();
  }

  void weather_init() {
    http_global_init();
  }

  string weather_content(const string& city) {
    auto uri = string("api.openweathermap.org/data/2.5/weather?q=") + city + "&appid=" + OpenWeatherMapKey;
    string response;
    auto get_result = http_get(uri , {{}}, response);
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
