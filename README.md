# RunDLL-NG

[![Build status](https://ci.appveyor.com/api/projects/status/nl12snf2j3uisyxw?svg=true)](https://ci.appveyor.com/project/BenjaminSoelberg/rundll-ng)

A replacement for RunDLL32 which works better than the buildin when working with malware.<br>

<pre>
RunDLL-NG, Version 1.2.0, (c) 2019 Benjamin Soelberg
----------------------------------------------------
Email   benjamin.soelberg@gmail.com
Github  https://github.com/BenjaminSoelberg/RunDLL-NG

Usage:  rundll32-ng.exe dllname <optional string arguments> <optional list of entrypoints>

Help:   rundll32-ng random.dll
        Will load random.dll

        rundll32-ng random.dll @SomeNamedImport
        Will load random.dll and execute named import "SomeNamedImport"

        rundll32-ng random.dll #1
        Will load random.dll and execute ordinal import 1

        rundll32-ng random.dll Some String Argument #1 #2 @SomeNamedImport
        Will load random.dll with argument "Some String Argument" and
        execute ordinal import 1, 2 and named import SomeNamedImport
</pre>
