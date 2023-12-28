# About
Noita Map Viewer generates an image of your world based on data from your save, with no ingame mods or settings fiddling required, unlike MapCap.
Now includes map painting! Switch to edit mode with Shift+E, then start painting away! Press Ctrl+S to save the edited chunks back to your save file.
Planned features include async rendering of chunks to bypass the startup loading time and filtering for specific materials.

# Usage
Simply extract the .zip from the most recent release (or build the program yourself if you're on Linux), and run the executable.
For Windows users, the program will attempt to automatically locate your save, although you can pass an alternate directory to /save00/ as the first command line argument.
Linux users always need to use the argument to provide a path to save00, since the directory can vary considerably.

Linux users also have to build the project from scratch as no linux release is provided at the moment. Prerequisites are fastlz, libpng, and sfml. If you can't get it to work, I might be able to help you but I am not a linuxer so you may be on your own.

# Credits
dexterCD - Reverse engineered all of the .png_petri format before I got to it, and did a much better job than I probably could have. Responsible for a majority of the save loading for this tool

charclr - Stoked a healthy sense of competition that convinced me to update at a reasonable rate.

Heinermann - His wiki tools were invaluable in generating colors for every material.

Copi and Nathan - Moral support
