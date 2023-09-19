## Build instructions

### Download Source code

Clone the project radeon_raytracing_analyzer from github.com
```bash
git clone https://github.com/GPUOpen-Tools/radeon_raytracing_analyzer.git
```

### Building on Windows
As a preliminary step, make sure that you have the following installed on your system:
* CMake 3.11 or above.
* Python 3.7 or above.
* Qt® 5 or above (5.15.2 is the default and recommended).
* Visual Studio® 2015 or above (2019 is the default).

Qt V5.15.2 can be installed using the Qt online installer available from the Qt 5.15.2 release page [here][qt-online].
As an alternative, the Qt 5.12.6 offline installer can be used [here][qt-offline].
Qt should be installed to the default location (C:\Qt\Qt5.xx.x).
Be sure to select msvc2017/msvc2019 64-bit during Qt installation, depending on the compiler you decide to use.
A reboot is required after Qt is installed.

CMake can be downloaded from [here](https://cmake.org/download/).
Python (V3.x) can be downloaded from [here](https://www.python.org/). To build the documentation from Visual Studio, the Sphinx Python Document Generator is needed.
This can be installed once Python is installed, as follows:
* Open a command prompt and navigate to the scripts folder in the python install folder. Then type these 2 commands:
* pip install -U sphinx
* pip install sphinx_rtd_theme

Run the python pre_build.py script in the build folder from a command prompt. If no command line options are provided, the defaults will be used (Qt 5.15.2 and Visual Studio 2019)

Some useful options of the pre_build.py script:
* --vs <Visual Studio version>: generate the solution files for a specific Visual Studio version. For example, to target Visual Studio 2017, add --vs 2017 to the command.
* --qt <path>: full path to the folder from where you would like the Qt binaries to be retrieved. By default, CMake would try to auto-detect Qt on the system.

Once the script has finished, in the case of Visual Studio 2019, a sub-folder called 'vs2019' will be created containing the necessary build files.
Go into the 'vs2019' folder (build/win/vs2019) and double click on the RRA.sln file and build the 64-bit Debug and Release builds.
The Release and Debug builds of RRA will be available in the build/release and build/debug folders.

### Building on Ubuntu
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
```
Qt V5.15.2 can be installed using the Qt online installer available from the Qt 5.15.2 release page [here][qt-online].
As an alternative, the Qt 5.12.6 offline installer can be used [here][qt-offline] (the .run file) and installed
to ~/Qt/Qt5.12.6 (the default of ~/Qt5.12.6 will not work).

XCB libraries are required for Qt v5.15.x (they are not needed for older Qt versions). By default, the CMake configuration will attempt to copy
these files from the Qt lib folder. If these files are installed elsewhere on the system or an older version of Qt is being used to build RRA,
the --disable-extra-qt-lib-deploy pre_build.py script argument may be used. This will prevent the build configuration scripts from attempting to copy
the libraries in the post build step. If needed, the XCB library files (libxcb*) can be obtained from the /lib folder of the Radeon Developer Tool
Suite download found [here](https://gpuopen.com/tools/).

Run the python pre_build.py in the build folder.
```bash
python3 pre_build.py
```
Or run the pre_build.py script with the -qt option to specify another version of Qt (also use the --disable-extra-qt-lib-deploy flag since the XCB
libraries aren't needed). For example:
```bash
python3 pre_build.py --qt 5.12.6 --disable-extra-qt-lib-deploy
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

It is possible to use the system-installed version of Qt rather than using a Qt installer described above. At the time of this writing, Ubuntu 22.04 LTS
comes with Qt 5.15.3. To use the system Qt, a fake Qt package is needed. For Qt 5.15.3, this can be made by creating the required directory structure
and setting up symbolic links to point to the system Qt lib and include directories:
```bash
mkdir -p ~/Qt/Qt5.15.3/5.15.3/gcc_64
sudo ln -s /usr/lib/x86_64-linux-gnu ~/Qt/Qt5.15.3/5.15.3/gcc_64/lib
sudo ln -s /usr/include/x86_64-linux-gnu/qt5 ~/Qt/Qt5.15.3/5.15.3/gcc_64/include
```
python3 pre_build.py --qt 5.15.3 --qt-system --disable-extra-qt-lib-deploy --build

Some additional Qt components may be required, so install those:

```
sudo apt-get install qtbase5-dev
sudo apt-get install qtbase5-dev-tools
sudo apt-get install libqt5svg5-dev
sudo apt-get install libqt5x11extras5
sudo apt-get install qtbase5-private-dev
```

[qt-online]: https://www.qt.io/blog/qt-5.15.2-released
[qt-offline]: https://download.qt.io/archive/qt/5.12/5.12.6/
