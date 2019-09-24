# ScenarioCreator

The main components of this part are based on the [ScenarioCreator](https://github.com/fabbrimatteo/JTA-Mods/tree/master/ScenarioCreator)-part of [JTA-Mods](https://github.com/fabbrimatteo/JTA-Mods). The functionality has been extended to be able to create scenes with more shown pedestrians, as the thesis focuses on crowded scenarios.

## Set-Up Steps

1. Open Visual Studio Code and select `File/New/Poject From Existing Code...` what will open a new window
2. Leave the defaultly selected `Visual C++` in the Dropdown menu for the type if the project and press `Next>`.
3. Choose `$(ROOT)/ScenarioCreator` as **Project File Location** and set the **Project Name** to `ScenarioCreator`. Press `Next>` then.
4. In the next window, select `Dinamically Linked Library (DLL) Project` in the **Project type** Dropdown menu and press `Finish`. The solution will then be visible in the **Solution Explorer**. Its name should the one specified in in step 3.
5. Under **Solution Configurations** select `Release` and under **Solution Plattform** select `x64`.
6. Click right on the displayed solution and include the the `inc` folder of the extracted 'ScriptHookV SDK' archive under `Configuration Properties->C/C++->General->Additional Include Directories`. Make sure here that the configuration and plattform are the ones specified in step 5.
7. Include the `lib` folder of the extracted 'ScriptHookV SDK' archive under `Configurations Properties->Linker->General->Additional Library Directories`.
8. Also include `ScriptHookV.lib` from the `lib` directory (together with the path) under `Configurations Properties->Linker->Input->Additional Dependencies`.
9. Under `Configuration Properties->General` change `Windows SDK Version` to `10.0.17763.0`, `Target Extension` to `.asi` and `Configuration Type` to `Dynamic Library (.dll)`
10. Build the solution and copy the generated file `ScenarioCreator.asi` in the exe-folder of GTA-V.
