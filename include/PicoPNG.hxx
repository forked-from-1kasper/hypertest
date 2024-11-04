#pragma once

#include <iostream>
#include <fstream>
#include <vector>

namespace PNG {
    extern std::vector<unsigned char> & load(const std::string &, unsigned long &, unsigned long &);
}