#ifndef UTILS_H
#define UTILS_H

#include <stdexcept>
#include <string>
#define CORt(x) if(!SUCCEEDED(x)) throw std::exception();

inline std::wstring towide(const std::string& str) { return std::wstring(str.begin(), str.end());  }

#endif