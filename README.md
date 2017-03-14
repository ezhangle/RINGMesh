[![Build status](https://ci.appveyor.com/api/projects/status/d2hkhxnlb4ke6gy0/branch/default?svg=true)](https://ci.appveyor.com/project/ArnaudBotella/ringmesh/branch/default)
[![Coverage Status](https://coveralls.io/repos/bitbucket/ring_team/ringmeshcoverage/badge.svg?branch=default)](https://coveralls.io/bitbucket/ring_team/ringmeshcoverage?branch=default)


Instructions for compiling RINGMesh
===================================

RINGMesh is tested under Linux (64 bits) and Windows (64 bits).
You will need CMake (version >= 2.8.11). There is no other dependency (everything
 you need is shipped with RINGMesh). Follow the Linux or Windows instructions below.

Linux
=====

Configuring RINGMesh
------------------

Execute cmake command in a RINGMesh/build directory.

* mkdir build
* cd build

To configure using default options:

* cmake ..

To define the options, use the cmake interface:

* cmake-gui .. or ccmake ..


Compiling RINGMesh
------------------

* cd build/ringmesh/Release
* make [-j4]

To build in debug, go to build/ringmesh/Debug instead.

Eclipse-cdt project is provided (.project and .cproject). You can import RINGMesh into
Eclipse: File>Import...>General>Existing Projects into Workspace. Click on next, then
select the root directory (RINGMesh directory) then click on Finish. There are two
build configurations: release and debug. There is a target to clean and compile. There are
also targets to build/rebuild geogram.

Compiling the documentation
---------------------------

The documentation can be generated using [Doxygen](http://www.stack.nl/~dimitri/doxygen/):

* Check the BUILD_DOCUMENTATION option when using cmake
 * cd build
 * ccmake ..
 * set BUILD_DOCUMENTATION option to ON
 * configure and generate
* cd build/ringmesh/Release
* make doc-devkit OR make doc-devkit-lite

doc-devkit include full documentation of RINGMesh and Geogram
doc-devkit-lite include only the RINGMesh documentation

Then you can go in doc/devkit[-lite]/html and open the index.html with your web browser

Windows
=======

Configuring RINGMesh
------------------

Launch CMake GUI, indicate where is the source code as the path to RINGMesh root and 
where to put the binaries as this_root/build/ringmesh.
Configuration options can be set in using the interface.

RINGMesh has previously been compiled with:

* Visual Studio 10 2010 Win64
* Visual Studio 11 2012 Win64
* Visual Studio 12 2013 Win64
* Visual Studio 14 2015 Win64

Compiling RINGMesh
------------------

You can either launch building in VisualStudio or calling cmake in command line
in the build directory created at the configuration step.
  cmake --build . --config Release

Compiling the documentation
---------------------------

The documentation can be generated using [Doxygen](http://www.stack.nl/~dimitri/doxygen/):

* Check the BUILD_DOCUMENTATION option when using cmake
* Open the solution which is in build/ringmesh/RINGmesh.sln in VisualStudio
* Build the doc-devkit or the doc-devkit-lite project

doc-devkit include full documentation of RINGMesh and Geogram
doc-devkit-lite include only the RINGMesh documentation

Then you can go in doc/devkit[-lite]/html and open the index.html with your web browser