# RunDLL-NG
A replacement for RunDLL32 which works better than the buildin when working with malware.<br>
This tool does nothing fancy like trying to locate exports using A and W extensions.<br>
It just uses LoadModule as well as GetProcAddress. Also running an export after dllmain is optional.<br>
