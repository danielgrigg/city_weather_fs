city\_weather\_fs
=================

Creates a FUSE filesystem of the world's cities and their weather forecast.

This is just a proof-of-concept, but it works pretty well.  I'm only
including 15K+ cities to make it human consumable.  Cities are aggregated
by country-code.

All written in c++11 and currently limited to Xcode but readily portable
to Linux.

Installing
==========

First install [OSXFuse](http://osxfuse.github.io)

Build the cityfs driver
  xcodebuild

Mount a cityfs filesystem:
  mkdir /tmp/foo
  cd build\Release
  ./cityfs /tmp/foo 

Explore the filesystem:
  cd /tmp/foo/AU
  cat Sydney
  cat Brisbane


## Acknowledgements

* openweathermap.org - weather data
* rapidjson (https://code.google.com/p/rapidjson/) for JSON parsing
* libcurl (http://curl.haxx.se/libcurl/) http networking

