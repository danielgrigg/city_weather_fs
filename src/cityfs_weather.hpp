//  Copyright (c) 2014 Daniel Grigg. All rights reserved.

#ifndef CITYFS_WEATHER_HPP
#define CITYFS_WEATHER_HPP

#include <string>

namespace cityfs {

  void weather_init();
  std::string weather_content(const std::string& city);
}

#endif
