#ifndef COMMONS_HPP
#define COMMONS_HPP

#include <algorithm>
#include <string>

#if defined(unix) || defined(__unix__) || defined(__unix)
#define _UNIX
#endif

std::string toLower(std::string str);

#endif