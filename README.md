# E-Sim

A mini EDA (Electronic Design Automation) tool for drawing electronic circuits and running SPICE simulations. Built with Qt6/C++ and the ngspice engine.

## Features (In Progress)
- Schematic capture with component placement and wire drawing
- DC Operating Point analysis
- Transient analysis with waveform plotting
- AC Sweep (frequency response)
- Interactive component value editing

## Tech Stack
- Qt 6.10+ (Widgets, Charts, Graphics View)
- ngspice 46 shared library
- C++17
- Qt Creator 19+

## Build Requirements
- Qt 6.10+ with Charts module
- ngspice 46 (64-bit DLL)
- Windows 10/11

## Setup
1. Download `ngspice.dll` from [ngspice](https://sourceforge.net/projects/ngspice/)
2. Place `ngspice.dll` in `lib/`
3. Place `sharedspice.h` in `include/ngspice/`
4. Open `CircuitSim.pro` in Qt Creator
5. Build and run

## Status
- Week 1: ✅ Qt6 project + ngspice init + DC operating point
- Week 2: ✅ Transient simulation + RC charging curve plot (QChart)
- Week 3: 🔄 AC Sweep (frequency response)