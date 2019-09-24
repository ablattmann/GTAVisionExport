**DatasetExporter set-up and build steps**
-------------------------------------

Test set-up:
----------

The repository is tested and should therefore work with the following setup:

+ **FIXME: add GPU** 
+ Windows 10 64bit
+ Visual Studio 2017 (enterprise edition, but should also work with community edition)
+ cmake-gui (version 3.15.3)
+ GTA-V version 1.0.231.0

Needed tools and libraries
--------------------------
+ [AsiLoader and ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)

+ [CMake](https://cmake.org/download/)

+ [MS Visual Studio](https://www.visualstudio.com/cs/downloads/) 


Set-up steps
-----------

In the following descriptions `$(ROOT)` will refer to the path to the root of this repository

1. Download and extract ScriptHookV archive and drop the files in 'bin' into your GTAV exe folder.
2. Download and extract the ScriptHookV SDK from the same page and extract the archive to some convenient place.
2. Start CMake (cmake-gui) from your Windows start menu.
   * Hit `Browse Source...` and select the folder `$(ROOT)/DatasetExporter` folder.
6. Hit `Browse Build...`, create `$(ROOT)/DatasetExporter/build` folder and select it.
7. Hit `Configure`.
8. In the window, will subsequently open, choose your Visual Studio Version as `project generator` (in my case, this was'Visual Studio 15 2017') and the fitting plattform for the architecture of your machine ('x64' in my case). The predefined option `use default native compilers` should be kept. The configuration should suceed after this step.
10. Run `Generate`, what should create the file _DatasetExporter.sln_ in your `$(ROOT)/DatasetExporter/build` folder. Open this newly created file afterwards.
13. Select the `Release`-option in the `Solution Configurations`-Drop-Down-Menu and make sure that the selected `Solution Plattform`-option (next to the `Solution Configurations`-Drop-Down-Menu) is compliant with your architecture.
14. Right click on `DatasetExporter` in the solution explorer side window and hit the `Properties`-option. In the opening window, choose `Configuration Properties/C-C++/Additional Include Dirs` and add the `./DatasetExporter/src` folder (this allows VS to find MinHook.h, which is required). For convenience, hit `apply` before continuing.
15. In the same window, under `Configuration Properties/Linker/Input/Additional Dependencies` add : 
`$(ROOT)\DatasetExporter\deps\libMinHook.x64.lib` 
16. Build the solution. it should now succeed and the products should be in `$(ROOT)\DatasetExporter\build\src\Release`
17. Copy _DatasetExporter.asi_ & _DatasetExporter.lib_ to your GTAV exe folder.
13. In the same folder, create a new directory `param` and copy `$(ROOT)/parameters.txt` to this new directory.
18. Run GTAV.
19. Get to a place where you want to grab frames and press 'l' (lowercase 'L') to grab a frame. GTAVisionExport should now create color.raw, stencil.raw and depth.raw files in your GTAV exe folder.

HTH

FAQ
---

Can not configure in CMake, `gdi32.lib` is missing:
This is probably due to incorrect Visual Studio SDK, can be solved by installing Windows 10 SDK (10.0.15063.0) for Desktop C++ x86 and x64 in the VS Installer. 

Source: https://stackoverflow.com/questions/33599723/fatal-error-lnk1104-cannot-open-file-gdi32-lib

The game crashes after pressing 'L':

If you are using steam, be sure to disable the steam overlay for this game.

If steam overlay is disabled and game still crashes, be sure to have resolution same or higher as constants in source code of this project.
Default is 1000 x 1000, as can be seen [here](https://github.com/umautobots/GTAVisionExport/blob/4f2bf90997df056764605076c0c95b885c424c43/native/src/main.cpp#L235) and [here](https://github.com/umautobots/GTAVisionExport/blob/4f2bf90997df056764605076c0c95b885c424c43/native/src/main.cpp#L212).
If you want to use lower resolution, change these numbers and recompile.
