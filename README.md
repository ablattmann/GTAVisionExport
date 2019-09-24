# DatasetCreator

Code to extract annotated images from GTAV. The annotations contain keypoints as well as semantic segmentation maps. Moreover, 
a tool is provided to create new scenes, which can be subsequently recorded. The repository 
is fuses and extends functionality from [GTAVisionExport](https://github.com/umautobots/GTAVisionExport) and [JTA-Mods](https://github.com/fabbrimatteo/JTA-Mods).

The repository is structured as follows:
* The `ScenarioCreator`-directory contains code for the creation of new scenes and their storing.
* The `native`-directory contains code to load the saved scenes and record these including keypoins- and semantic-maps-annotations.

## Test set-up:

The repository is tested with the following setup:

+ **FIXME: add GPU** 
+ Windows 10 64bit
+ Visual Studio 2017 (enterprise edition, but should also work with community edition) and Visual Studio 2019 
+ cmake-gui (version 3.15.3)
+ GTA-V version 1.0.231.0

## Needed tools and libraries

+ [AsiLoader and ScriptHookV](http://www.dev-c.com/gtav/scripthookv/): Download the archive containing the Asi Loader and Native Trainer as well as the SDK an extract them.

+ [CMake](https://cmake.org/download/): Install the windows version (I used the installer that's provided under `Binary Distributions`)

+ [MS Visual Studio](https://www.visualstudio.com/cs/downloads/): Download and install

# TODO

* Einstellungen in GTA erklären: Borderless Window Mode Disable HDU und Radar, 
* Erklären, wo welche Files hinmüssen
* asis bereitstellen

