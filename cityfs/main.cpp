//
//  main.cpp
//  cityfs
//
//  Created by Daniel Grigg on 13/01/2014.
//  Copyright (c) 2014 Daniel Grigg. All rights reserved.
//

#include <iostream>
#include <fuse.h>
#include <unordered_map>
#include <string>
#include <vector>

using namespace std;
vector<string> cities = { "Brisbane", "Adelaide", "Sydney", "Melbourne", "Perth", "Hobart", "Canberra", "Darwin" };

int main(int argc, const char * argv[])
{
  std::cout << "Starting cityfs!\n";
  
  return 0;
}

