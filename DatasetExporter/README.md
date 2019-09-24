**DatasetExporter set-up and build steps**
-------------------------------------

This repostory extends the functionality of the `native` part of [GTAVisionExport](https://github.com/umautobots/GTAVisionExport/tree/master/native). In addition to the extracted semantic segmentations, there are now keypoints annotations available. Moreover, it is possible to record entire video sequences and not only single frames.

Set-up steps
-----------

In the following descriptions `$(ROOT)` will refer to the path to the root of this repository

1. Download and extract ScriptHookV archive and drop the files in 'bin' into your GTAV exe folder.
2. Download and extract the ScriptHookV SDK from the same page and extract the archive to some convenient place.
3. Start CMake (cmake-gui) from your Windows start menu.
   * Hit `Browse Source...` and select the folder `$(ROOT)/DatasetExporter` folder.
4. Hit `Browse Build...`, create `$(ROOT)/DatasetExporter/build` folder and select it.
5. Hit `Configure`.
6. In the window, will subsequently open, choose your Visual Studio Version as `project generator` (in my case, this was 'Visual Studio 15 2017' and 'Visual Studio 16 2019') and the fitting plattform for the architecture of your machine ('x64' in my case). The predefined option `use default native compilers` should be kept. The configuration should suceed after this step.
7. Run `Generate`, what should create the file _DatasetExporter.sln_ in your `$(ROOT)/DatasetExporter/build` folder. Open this newly created file afterwards.
8. Select the `Release`-option in the `Solution Configurations`-Drop-Down-Menu and make sure that the selected `Solution Plattform`-option (next to the `Solution Configurations`-Drop-Down-Menu) is compliant with your architecture.
9. Right click on `DatasetExporter` in the solution explorer side window and hit the `Properties`-option. In the opening window, choose `Configuration Properties/C-C++/Additional Include Dirs` and add the `./DatasetExporter/src` folder (this allows VS to find MinHook.h, which is required). For convenience, hit `apply` before continuing.
10. In the same window, under `Configuration Properties/Linker/Input/Additional Dependencies` add : 
`$(ROOT)\DatasetExporter\deps\libMinHook.x64.lib` 
11. Build the solution. it should now succeed and the products should be in `$(ROOT)\DatasetExporter\build\src\Release`
12. Copy _DatasetExporter.asi_ & _DatasetExporter.lib_ to your GTAV exe folder.
13. In the same folder, create a new directory `param` and copy `$(ROOT)/parameters.txt` to this new directory. Also copy the example-scenario-folder the the GTA-V exe folder. This will allow you to test the recording with some default scenarios. 
14. Run GTAV.
15. By pressing `F3`-key, a new recording session can be started, if there are scenarios avalailable. The recording session ends, if there are no scenes available, that have not been recorded so far.

