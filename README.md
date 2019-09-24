# DatasetCreator

Code to extract annotated images from GTAV. The annotations contain keypoints as well as semantic segmentation maps. Moreover, 
a tool is provided to create new scenes, which can be subsequently recorded. The repository 
is fuses and extends functionality from [GTAVisionExport](https://github.com/umautobots/GTAVisionExport) and [JTA-Mods](https://github.com/fabbrimatteo/JTA-Mods).

The repository is structured as follows:
* The `ScenarioCreator`-directory contains code for the creation of new scenes and their storing.
* The `native`-directory contains code to load the saved scenes and record these including keypoins- and semantic-maps-annotations.
