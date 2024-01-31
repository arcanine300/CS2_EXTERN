# CS2_EXTERN
Read-only external CS2 base utilizing overlay hijacking &amp; remote IPC

# Compile Instructions

The main application can be compiled and ran like a normal application. Making edits to the main application such as changing offsets doesn't require you to recompile RemoteRPM but if you
 wish to make changes to RemoteRPM i.e shellcode payload you will need to 
- compile RemoteRPM.sln
- Download [Donut](https://github.com/TheWover/donut?tab=readme-ov-file)
- move remoteRPM.exe to directory with donut.exe
- use donut to generate shellcode (loader.bin) `donut -i "remoteRPM.exe"`
- embed shellcode into CS2_EXTERN.sln as a [PE Resources](https://www.ired.team/offensive-security/code-injection-process-injection/loading-and-executing-shellcode-from-portable-executable-resources)

## Requirements
A Nvidia GPU with the Nvidia Geforce Overlay installed, Windows 10 / 11

## Showcase

![Image1](https://github.com/arcanine300/CS2_EXTERN/assets/24901604/42ca9d20-36f1-4777-bd10-b9c07f308651)
