#!/bin/bash
cd ..
git log --pretty=format:'#define GIT_INFO_PRESENT%n static const char* GIT_HASH = "%H";' -n 1 > src/gitversion.hpp
