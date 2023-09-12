#!/bin/bash
cd "$(dirname "$0")"
cd ../../STMViewer/
ls
git log --pretty=format:'static const char* GIT_HASH = "%H";' -n 1 > src/gitversion.hpp
