#pragma once

#include <vector>
#include <string>

namespace PNG {
    extern std::vector<unsigned char> & load(const std::string &, unsigned long &, unsigned long &);
}