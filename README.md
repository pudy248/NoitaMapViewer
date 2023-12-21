# About
Noita Map Viewer generates an image of your world based on data from your save, with no ingame mods or settings fiddling required, unlike MapCap. 
Planned features include async rendering of chunks to bypass the startup loading time, filtering for specific materials, and the ability to paint materials on the map and save them to your Noita save file.

Controls: 
- WASD/Arrow Keys - Camera panning
- Scroll Wheel/+- - Zoom
- Shift - Faster panning
- Alt - Toggle coordinate tooltip
- CTRL+S - Save map as image

# Usage
Simply extract the .zip from the most recent release (or build the program yourself if you're on Linux), and run the executable.
For Windows users, the program will attempt to automatically locate your save, although you can pass an alternate directory to /save00/ as the first command line argument.
Linux users always need to use the argument to provide a path to save00, since the directory can vary considerably.

# Credits
dexterCD - Reverse engineered all of the .png_petri format before I got to it, and did a much better job than I probably could have. Responsible for a majority of the save loading for this tool

Heinermann - His wiki tools were invaluable in generating colors for every material.

Copi and Nathan - Moral support
