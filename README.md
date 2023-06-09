# About
Noita Map Viewer generates an image of your world based on data from your save, with no ingame mods or settings fiddling required, unlike MapCap. 
Currently, most config options are still WIP, the program only loads chunks from the main world (a 70x48 chunk area), and there are no extra settings or ways to edit the save.
Planned features include async rendering of chunks to bypass the startup loading time, filtering for specific materials, re-addition of box2d object rendering, and the ability to paint materials on the map and save them to your Noita save file.

# Usage
Simply extract the .zip from the most recent release (or build the program yourself if you're on Linux, prebuilt binaries pending), and run the executable. Then, WASD/arrow keys can be used for navigation and the scroll wheel zooms.
For Windows users, the program will attempt to automatically locate your save, although you can pass an alternate directory to /save00/ as the first command line argument.
Linux users always need to use the argument to provide a path to save00, since the directory can vary considerably.

# Credits
dexterCD - Reverse engineered all of the .png_petri format before I got to it, and did a much better job than I probably could have. Responsible for a majority of the save loading for this tool
Heinermann - His wiki tools were invaluable in generating colors for every material.
Copi and Nathan - Moral support
