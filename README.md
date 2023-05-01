# STMViewer 

An open source GUI tool for viewing and manipulating variables data using debug interface and st-link programmer on STM32 microcontrollers.

## Introduction

STMViewer can be used to visualize your embedded application data in real time with no overhead in a non-intrusive way. The software works by reading varaibles' values directly from RAM using the debug interface. Addresses are read from the *.elf file created when you build your embedded project. This approach main downside is that the object's address must stay constant throughout the whole program lifetime, which means it has to be global. Even though it seems to be a small price to pay in comparison to running some debug protocol over for example UART which is also not free in terms of intrusiveness.

STMViewer is a great tool for debugging, but might be of little use with release builds (which usually lack debug info). 

## Why
I'm working in the motor control industry where it is crucial to visualize some of the process data in real-time. Since The beginning I was working with [STMStudio](https://www.st.com/en/development-tools/stm-studio-stm32.html), which is, or rather was, a great tool. Unfortunately ST stopped supporting it which means there are some annoying bugs, and it doesnt work well with mangled c++ object names. Also it works only on Windows which is a big downside. If you've ever used it you probably see how big of an inspiration it was for creating STMViewer :) ST's another project in this area - [Cube Monitor](https://www.st.com/en/development-tools/stm32cubemonitor.html) - is simply underdeveloped, and seems to be abandoned as well so it is simply useless. 

## Installation

