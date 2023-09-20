![example workflow](https://github.com/klonyyy/STMViewer/actions/workflows/build.yaml/badge.svg)

# STMViewer 
STMViewer is an open-source GUI debug tool for STM32 microcontrollers that consists of two modules:
1. Variable Viewer - used for viewing, logging, and manipulating variables data in realtime using debug interface (SWDIO / SWCLK / GND)
2. Trace Viewer - used for graphically representing real-time SWO trace output (SWDIO / SWCLK / SWO / GND)

The only piece of hardware required is an ST-Link programmer. 

## Introduction

### Variable Viewer
![_](./docs/VarViewer.gif)

Variable Viewer can be used to visualize your embedded application data in real time with no overhead in a non-intrusive way. The software works by reading variables' values directly from RAM using the ST-link programmer debug interface. Addresses are read from the *.elf file which is created when you build your embedded project. This approach's main downside is that the object's address must stay constant throughout the whole program's lifetime, which means the object has to be global. Even though it seems to be a small price to pay in comparison to running some debug protocol over for example UART which is also not free in terms of intrusiveness.

Variable Viewer is a great tool for debugging, but might be of little use with highly optimized release builds (which usually lack debug info), or very fast-changing signals.

### Trace Viewer 
![_](./docs/TraceViewer.gif)

Trace Viewer is a new module that lets you visualize SWO trace data. It can serve multiple purposes such as profiling a function execution time, confirming the timer's interrupt frequency, or displaying very fast signals. All this is possible thanks to hardware trace peripherals embedded into Cortex M3/M4/M7/M33 cores. For prerequisites and usage please see the Quick Start section. 

TraceViewer is not influenced by optimizations, which means it is a great tool to use for profiling on release builds. Moreover it has a very low influence on the program execution as each datapoint is a single register write. 

## Installation

Linux: 
1. Download the *.deb package and install it using:
`sudo apt install ./STMViewer-x.y.z-Linux.deb`
All dependencies should be installed and you should be ready to go. 
Optional: make sure you have the rights to access usb port. When installing ST's software such as Cube Programmer it will most probably also install needed udev rules.

Windows: 
1. Make sure you've got GDB installed (v12.1 or later) and added to your PATH (the easiest way is to install using [MinGW](https://www.mingw-w64.org))
2. Download and run the STMViewer installer. Make sure the ST-link is in "STM32 Debug + Mass Storage + VCP" mode as for some reason "STM32 Debug + VCP" throws libusb errors on Windows. This needs further investigation. 

You can assign the external GPU to STMViewer for improved performance. 

## Quick Start 

### Variable Viewer
1. Open Options->Acqusition Settings window in the top menu. 
2. Select your project's elf file. Make sure the project is compiled in debug mode. Click done. 
3. Click the 'add variable' button to add a new variable. Double-click to change its name to one of your global variables. If you're using structs or classes in C++ make sure to add its name before the variable, exactly like you'd refer to it in the code (for example myClass.var, or namespace::myClass.var). 
4. After adding all variables click 'update variable addresses'. The type and address of the variables you've added should change from "NOT FOUND!" to a valid address based on the *.elf file you've provided.
5. Drag and drop the variable to the plot area.
6. Make sure the ST-Link is connected. Download your executable to the microcontroller and press the "STOPPED" button. 

In case of any problems, please try the test/STMViewer_test CubeIDE project and the corresponding STMViewer_test.cfg project file. Please remember to build the project and update the elf file path in the Options -> Acquisition Settings. 

Example project with STMViewer config file is located in test/STMViewer_test directory.

### Trace Viewer 
1. Turn on the SWO pin functionality - in CubeMX System Core -> SYS Mode and Configuration -> choose Trace Asynchronous Sw
2. Place enter and exit markers in the code you'd like to profile. Example for digital data: 
```
ITM->PORT[x].u8 = 0xaa; //enter tag 0xaa - plot state high
foo();
ITM->PORT[x].u8 = 0xbb; //exit tag 0xbb - plot state low
```
And for tracing "analog" signals you can use: 
```
float a = sin(10.0f * i);          // some super fast signal to trace
ITM->PORT[x].u32 = *(uint32_t*)&a; // type-punn to desired size: sizeof(float) = sizeof(uint32_t)
```
or

```
uint16_t a = getAdcSample();       // some super fast signal to trace
ITM->PORT[x].u16 = a;              
```

The ITM registers are defined in CMSIS headers so no additional includes should be necessary.

3. Compile and download the program to your STM32 target.
4. In the `Settings` window type in the correct System Core Clock value in kHz (very important as it affects the timebase)
5. Try different trace prescallers that result in a trace speed lower than the max trace speed of your programmer (for example STLINK V2 can read trace up to 2Mhz, whereas ST-Link V3 is theoretically able to do 24Mhz). Example:
- System Core Clock is 160 000 kHz (160 Mhz)
- We're using ST-link V2 so the prescaler should be at least 160 Mhz / 2 Mhz = 80
6. Configure "analog" channels types according to the type used in your code. 
7. Press the "STOPPED" button to start recording.

Example project with STMViewer config file is located in test/STMViewer_test directory.

FAQ and common issues: 
1. Problem: My trace doesn't look like it's supposed to and I get a lot of error frames
Answer: Try lowering the trace prescaller and check the SWO pin connection - the SWO pin output is a fast signal and it shouldn't be too long.

2. Problem: My trace looks like it's supposed to but I get the "delayed timestamp 3" indicator
Answer: Try logging fewer channels simultaneously. It could be that you've saturated the SWO pin bandwidth.

3. Problem: My trace looks like it's supposed to but I get the "delayed timestamp 1" indicator
Answer: This is not a critical error, however, you should be cautious as some of the trace frames may be delayed. To fix try logging fewer channels simultaneously.


## Why
I'm working in the motor control industry where it is crucial to visualize some of the process data in real-time. Since the beginning, I have been working with [STMStudio](https://www.st.com/en/development-tools/stm-studio-stm32.html), which is, or rather was a great tool. Unfortunately, ST stopped supporting it which means there are some annoying bugs, and it doesn't work well with mangled c++ object names. Also, it works only on Windows which is a big downside. If you've ever used it you probably see how big of an inspiration it was for creating STMViewer :) ST's other project in this area - [Cube Monitor](https://www.st.com/en/development-tools/stm32cubemonitor.html) - has, in my opinion, too much overhead on adding variables, plots and writing values. I think it's designed for creating dashboards, and thus it serves a very different purpose. On top of that, I think the plot manipulation is much worse compared to STMStudio or STMViewer. 

Since the Trace Viewer module was added STMViewer has a unique property of displaying SWO trace data which both CubeMonitor and STMStudio currently lack. 

## 3rd party projects used in STMViewer

1. [stlink](https://github.com/stlink-org/stlink)
2. [libusb](https://github.com/libusb/libusb)
3. [imgui](https://github.com/ocornut/imgui)
4. [implot](https://github.com/epezent/implot)
5. [mINI](https://github.com/pulzed/mINI)
6. [nfd](https://github.com/btzy/nativefiledialog-extended)
7. [spdlog](https://github.com/gabime/spdlog)

