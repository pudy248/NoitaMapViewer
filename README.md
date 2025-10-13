# About
Noita Map Viewer generates an image of your world based on data from your save, with no ingame mods or settings fiddling required, unlike MapCap.
Now includes map painting! Switch to edit mode with Shift+E, then start painting away! Press Ctrl+S to save the edited chunks back to your save file.

# Usage
Simply download the [release.zip](https://github.com/pudy248/NoitaMapViewer/releases/latest/download/NoitaMapViewer_Win64_Release.zip) folder, extract it and run the executable. Note that the save shouldn't be open while doing this, as the map viewer can't process autosave files.
For Windows users, the program will attempt to automatically locate your save, although you can pass an alternate directory to /save00/ as the first command line argument.  
Linux users will have to run the command using Noitas wine prefix. You usually don't need to install wine seperately, as you can use the same Proton installation that steam uses for running Noita on linux in the first place: `WINEPREFIX=~/.local/share/Steam/steamapps/compatdata/881100/pfx ~/.local/share/Steam/steamapps/common/<Proton Version>/dist/bin/wine64 NoitaMapViewer.exe`.

# Credits
dexterCD - Reverse engineered all of the .png_petri format before I got to it, and did a much better job than I probably could have. Responsible for a majority of the save loading for this tool

charclr - Stoked a healthy sense of competition that convinced me to update at a reasonable rate.

Heinermann - His wiki tools were invaluable in generating colors for every material.

Copi and Nathan - Moral support
