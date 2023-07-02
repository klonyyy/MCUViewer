@echo off
cd ..
for /f "usebackq delims=" %%I in (`git rev-parse HEAD`) do set "gitHash=%%I"
echo #define GIT_INFO_PRESENT>nul > src\gitversion.hpp
echo static const char* GIT_HASH = "%gitHash%";>> src\gitversion.hpp
