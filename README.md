# Syntax-Highlight
Plugin for Altap Salamander (OpenSource)

## Requirements
For the plugin to work correctly, you need to add the folders `src-highlite-rel_3.1.8` and `lib` to the directory where the plugin is created (Debug/Release). 
An example of where to place these folders looks like this: `\SyntaxHighlight\vcproj2019\vc2019\Release_x64\plugins\SyntaxHighlight\src-highlite-rel_3.1.8`.

Alternatively, you can download and build the program from the official website: src-highlite-rel_3.1.8 -- (https://www.gnu.org/software/src-highlite/).

The `lib` folder contains the necessary libraries for the proper functioning of the plugin.

## How the Plugin Works

When the user presses F3, a window is displayed showing the source code they want to view. 
The plugin uses HTML structures to render the code.

To highlight the syntax, the plugin uses Source-Highlight. If syntax highlighting is not selected in the configuration window, the internal viewer will open instead.

The plugin can be configured in the configuration window (Ctrl+Shift+H).
