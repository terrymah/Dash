#ifndef UTILS_H
#define UTILS_H

#include <stdexcept>

#define CORt(x) if(!SUCCEEDED(x)) throw std::exception();

#endif