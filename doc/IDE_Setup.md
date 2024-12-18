# IDE/Source-Code-Editor Setup
## Visual Studio Code (Recommended)
Visual Studio Code is a source code editor by Microsoft. It can be downloaded at https://code.visualstudio.com/.

### First Startup
After cloning the repository open up the folder in Visual Studio Code (*File* > *Open Folder*).

### Installing Necessary Extentions
VS Code requires the C/C++ extention to be installed for advanced editor features (auto-complete, syntax error highlighting etc.). To install it open the "Extentions" panel on the left and install the "C/C++" extention by Microsoft.

### Configuring the C/C++ Extention
Open the *Command Palette* by pressing [F1] and choose the option *C/C++: Edit Configurations (UI)*.

In the opened tab scroll down to *Include Path*. Here you will have to enter two paths (separated by line breaks):

```
${workspaceFolder}/**
```
This path lets the plugin recognize all C++ files that are located in the voxie repository itself.

```
/usr/include/x86_64-linux-gnu/qt5/**
```
This is the path to the include files for the Qt library. The path might be different on your system, so check using the file explorer before setting this configuration.

### Setting the Project Build and Debug Configuration
VS Code should have created a ".vscode" folder inside the Voxie project directory (create it if it hasn't). Inside the folder create two files called "launch.json" and "tasks.json" (or edit them if they already exist, deleting the contents that were in them and replacing it with the text below).

The content of "launch.json" should be:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(gdb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build-debug/src/Main/voxie",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build Voxie (debug)"
        }
    ]
}
```

The content of "tasks.json" should be:
```json
{
    "tasks": [
        {
            "type": "shell",
            "label": "Build Voxie (debug)",
            "command": "${workspaceFolder}/tools/build",
            "args": [
                "--debug"
            ],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "clear": true
            }
        }
    ],
    "version": "2.0.0"
}
```

### Usage

Now you can build Voxie using *Terminal* > *Run Task* or by pressing [CTRL] + [SHIFT] + [B] and you can run/debug Voxie using *Run* > *Start Debugging* or by pressing [F5].
