# LocalTalk Analyzer

This analyzer decodes Apple LocalTalk traffic.

When connected to the RS-422 RX lines of a LocalTalk adapter with the help of a suitable differential receiver, the analyzer will track the LLAP framing used by LocalTalk, outputting the data bytes contained within each frame, from the first byte after the start flags, to the last byte of the CRC before the end flag.

Data from the analyzer may be exported and converted to pcap format for analysis in a tool such as Wireshark with the encoded python script `csv_to_pcap.py`.

## Getting Started

### MacOS

Dependencies:
- XCode with command line tools
- CMake 3.13+

Installing command line tools after XCode is installed:
```
xcode-select --install
```

Then open XCode, open Preferences from the main menu, go to locations, and select the only option under 'Command line tools'.

Installing CMake on MacOS:

1. Download the binary distribution for MacOS, `cmake-*-Darwin-x86_64.dmg`
2. Install the usual way by dragging into applications.
3. Open a terminal and run the following:
```
/Applications/CMake.app/Contents/bin/cmake-gui --install
```
*Note: Errors may occur if older versions of CMake are installed.*

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Ubuntu 16.04

Dependencies:
- CMake 3.13+
- gcc 4.8+

Misc dependencies:

```
sudo apt-get install build-essential
```

Building the analyzer:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### Windows

Dependencies:
- Visual Studio 2015 Update 3
- CMake 3.13+

**Visual Studio 2015**

*Note - newer versions of Visual Studio should be fine.*

Setup options:
- Programming Languages > Visual C++ > select all sub-components.

Note - if CMake has any problems with the MSVC compiler, it's likely a component is missing.

**CMake**

Download and install the latest CMake release here.
https://cmake.org/download/

Building the analyzer:
```
mkdir build
cd build -A x64
cmake ..
```

Then, open the newly created solution file located here: `build\localtalk_analyzer.sln`


## Output Frame Format

### Frame Type: `"data"`

| Property | Type | Description |
| :--- | :--- | :--- |
| `data` | int | Localtalk data byte |

This is the decoded localtalk byte
