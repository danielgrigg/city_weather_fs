# CityWeather FS

FUSE for the weather of cities across the globe.

Demonstrates the simplicity of creating a virtual filesystem 
using FUSE and hopefully inspires people to 'FUSE all the things!'.

Checkout the associated blog post at [CityFS](http://danielgrigg.github.io)!


## Installing

### MacOS

MacOS You'll need an updated FUSE installation which you can
get here [OSXFuse](http://osxfuse.github.io).   

### Linux  

    sudo apt-get install libcurl4-openssl-dev libfuse-dev

### Windows  

Unfortunately FUSE support is lacking.  There's some OSS efforts but you'll 
probably want to look at either writing your own Mini driver 
or a commercial solution like CBFS.


### Build the driver

    $ mkdir build; cd build
    $ cmake ..
    $ make


## Usage

I've supplied ./cities15k.csv which includes the most populous 15K cities
to populate the VFS.

Like most FUSE drivers you'll run cityfs which in turn mounts
the VFS to a given mount-point. Once the mount is complete, you 
can access the file system there.

    $ ./build/cityfs [city-file] [mount-point]

city-file must be a csv of (country-code,city,lat,lng,elevation,region)

  For example:
      $ cityfs cities15k.csv ~/cities

  Where cities15k.csv looks like:

      AU,Gold Coast,-28.00029,153.43088,591473,Australia/Brisbane
      AU,Gladstone,-23.84761,151.25635,30489,Australia/Brisbane
      AU,Geelong,-38.14711,144.36069,226034,Australia/Melbourne

mount-point must exist before launch

Once it's running, take try reading the file-tree under your mount-point.


## Code Layout

All the code is standard C++11.  CMake is used for builds.  There's no 
tests cause this is a POC ;)

+ driver.cpp - FUSE related code
+ cityfs.x - model the city FS
+ cityfs\_weather.x - OpenWeatherMap reader
+ country\_codes - precomputed country code -> country names
+ cityfs\_util - utility methods


## Acknowledgements

+ openweathermap.org - weather data
+ rapidjson (https://code.google.com/p/rapidjson/) for JSON parsing
+ libcurl (http://curl.haxx.se/libcurl/) http networking
+ osxfuse (http://osxfuse.github.io) of course for FUSE on OSX

Copyright(c) 2014 Daniel C Grigg
