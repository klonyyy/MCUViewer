# STMViewer 

An open source GUI tool for viewing and manipulating variables data using debug interface and st-link programmer on STM32 microcontrollers.

## Introduction

STMViewer can be used to visualize your embedded application data in real time with no overhead in a non-intrusive way. The software works by reading varaibles' values directly from RAM using the debug interface. Addresses are read from the *.elf file created when you build your embedded project. This approach main downside is that the object's address must stay constant throughout the whole program lifetime, which means it has to be global. Even though it seems to be a small price to pay in comparison to running some debug protocol over for example UART which is also not free in terms of intrusiveness.

STMViewer is a great tool for debugging, but might be of little use with release builds (which usually lack debug info). 

## Installation

Linux: 
Download the *.deb package and install it using:
`sudo apt install ./STMViewer-x.y.z-Linux.deb`

Windows: 
Download and run the installer.

## Quick Start

1. Open Options->Acqusition Settings window in the top menu. 
2. Select your project's elf file. Make sure the project is compiled in debug mode. Click done. 
3. Click 'add variable' button to add new variable. Double click to change it's name to one of your global variables. If youre using structs or classes in C++ make sure to add it's name before the variable, exactly like you'd refer to it in the code (example myClass.var). 
4. After adding all varaibles click 'update variable addresses'. The type and address of the varaibles you've added should change based on the *.elf file you've provided.
5. Click the plus sign below the varaibles table to add a plot. Drag and drop varaibles which you want to monito to the plot. 
6. Make sure the ST-Link is connected. Download your executable to the microcontroller and press the "STOPPED" button. 

## Why
I'm working in the motor control industry where it is crucial to visualize some of the process data in real-time. Since The beginning I was working with [STMStudio](https://www.st.com/en/development-tools/stm-studio-stm32.html), which is, or rather was, a great tool. Unfortunately ST stopped supporting it which means there are some annoying bugs, and it doesnt work well with mangled c++ object names. Also it works only on Windows which is a big downside. If you've ever used it you probably see how big of an inspiration it was for creating STMViewer :) ST's another project in this area - [Cube Monitor](https://www.st.com/en/development-tools/stm32cubemonitor.html) - is simply underdeveloped, and seems to be abandoned as well so it is simply useless. 

