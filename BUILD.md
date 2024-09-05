## Build instructions

To build RRA from source, third party software will need to be installed on your system.  A script may be provided that, when executed by you, automatically fetches and installs software onto your system, where such software originates from (a) elsewhere on your system, if such software is already present on your system, or (b) the internet, if such software is not already present on your system. BY EXECUTING THE SCRIPT AND INSTALLING THE THIRD PARTY SOFTWARE, YOU AGREE TO BE BOUND BY THE LICENSE AGREEMENT(S) APPLICABLE TO SUCH SOFTWARE. You agree to carefully review and abide by the terms and conditions of all license(s) that govern such software.  You acknowledge and agree that AMD is not distributing to you any of such software and that you are solely responsible for the installation of such software on your system.

### Download Source code

Clone the project radeon_raytracing_analyzer from github.com
```bash
git clone https://github.com/GPUOpen-Tools/radeon_raytracing_analyzer.git
```

### Get Qt6
Qt V6.7.0 can be installed using the Qt online installer available from the Qt 6.7.0 release page [here][qt-online].
As an alternative, the Qt 6.7.0 offline installer can be used [here][qt-offline].
Packages for Windows and Linux are provided.

### Building on Windows
As a preliminary step, make sure that you have the following installed on your system:
* CMake 3.11 or above.
* Python 3.7 or above.
* Qt® 6 or above (6.7.0 is the default and recommended).
* Visual Studio® 2019 or above (2022 is the default).

Qt should be installed to the default location (C:\Qt\Qt6.xx.x).
Be sure to select msvc2017/msvc2019 64-bit during Qt installation, depending on the compiler you decide to use.
Select msvc2019 if using Visual Studio 2022.
A reboot is required after Qt is installed.

CMake can be downloaded from [here](https://cmake.org/download/).
Python (V3.x) can be downloaded from [here](https://www.python.org/). To build the documentation from Visual Studio, the Sphinx Python Document Generator is needed.
This can be installed once Python is installed, as follows:
* Open a command prompt and navigate to the scripts folder in the python install folder. Then type these 2 commands:
* pip install -U sphinx
* pip install sphinx_rtd_theme

Run the python pre_build.py script in the build folder from a command prompt. If no command line options are provided, the defaults will be used (Qt 6.7.0 and Visual Studio 2022)

Some useful options of the pre_build.py script:
* --vs <Visual Studio version>: generate the solution files for a specific Visual Studio version. For example, to target Visual Studio 2017, add --vs 2017 to the command.
* --qt <path>: full path to the folder from where you would like the Qt binaries to be retrieved. By default, CMake would try to auto-detect Qt on the system.

Once the script has finished, in the case of Visual Studio 2022, a sub-folder called 'vs2022' will be created containing the necessary build files.
Go into the 'vs2022' folder (build/win/vs2022) and double click on the RRA.sln file and build the 64-bit Debug and Release builds.
The Release and Debug builds of RRA will be available in the build/release and build/debug folders.

### Building on Ubuntu
If Qt is installed from a Qt installer, it should be installed to ~/Qt/Qt6.7.0 (the default of ~/Qt6.7.0 will not work).

Required dependencies can be installed as follows:
```bash
sudo apt-get update
sudo apt-get install build-essential python3 chrpath
sudo apt-get install python3-pip
pip install sphinx_rtd_theme
sudo snap install cmake --classic
sudo apt-get install git
sudo apt-get install git-lfs
sudo apt-get install python3-sphinx
sudo apt-get install libxcb-xinerama0
sudo apt-get install mesa-common-dev libglu1-mesa-dev
sudo apt install libtbb-dev
```

Qt6 can be installed from the package manager using:
```bash
sudo apt-get install qt6-base-dev
sudo apt-get install qt6-base-private-dev
```
As of this writing, this package on Ubuntu 2204 is 6.2.4

XCB libraries are required for Qt v5 and above. These can be installed by using:
```bash
sudo apt-get install libxcb-cursor-dev
```

Run the python pre_build.py in the build folder.
```bash
python3 pre_build.py
```
Or run the pre_build.py script with the -qt option to specify another version of Qt. For example:
```bash
python3 pre_build.py --qt 6.7.0
```
The pre_build.py script will construct the output folders and build the necessary makefiles.
To build the release build, use:
```bash
make -j5 -C linux/make/release
```
Similarly for the debug build, use:
```bash
make -j5 -C linux/make/debug
```
Alternatively, building can be done directly from the prebuild script with the --build option
```bash
python3 pre_build.py --build
```

If Qt is not installed from a Qt installer, a fake Qt package is needed. This can be made by creating the required directory structure
and setting up symbolic links to point to the system Qt lib and include directories:
```bash
mkdir -p ~/Qt/Qt6.7.0/6.7.0/gcc_64
sudo ln -s /usr/lib/x86_64-linux-gnu ~/Qt/Qt6.7.0/6.7.0/gcc_64/lib
sudo ln -s /usr/include/x86_64-linux-gnu/qt6 ~/Qt/Qt6.7.0/6.7.0/gcc_64/include
```
python3 pre_build.py --qt 6.7.0 --build

[qt-online]: https://www.qt.io/blog/qt-6.7-released
[qt-offline]: https://download.qt.io/archive/qt/6.7/6.7.0
