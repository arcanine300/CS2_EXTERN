# CS2_EXTERN
Read-only external CS2 base utilizing overlay hijacking &amp; remote IPC 

# Compile Instructions
The main application can be compiled and ran like a normal application. Making changes such as changing offsets you will not need to recompile RemoteRPM but if you
 wish to make changes to RemoteRPM i.e shellcode payload you will need to 
- compile RemoteRPM.sln
- Download [Donut](https://github.com/TheWover/donut?tab=readme-ov-file)
- move remoteRPM.exe to directory with donut.exe
- use donut to generate shellcode (loader.bin) `donut -i "remoteRPM.exe"`
- embed shellcode into CS2_EXTERN.sln as a [PE Resources](https://www.ired.team/offensive-security/code-injection-process-injection/loading-and-executing-shellcode-from-portable-executable-resources)
