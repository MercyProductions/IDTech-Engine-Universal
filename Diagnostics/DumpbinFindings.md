# id Tech Dumpbin Findings

Generated from:

```text
C:\Program Files (x86)\Steam\steamapps\common\fvi
C:\Program Files (x86)\Steam\steamapps\common\Wolfenstein Enemy Territory
C:\Program Files (x86)\Steam\steamapps\common\Quake II RTX
```

## Wolfenstein Enemy Territory

```text
etmain\qagame_mp_x86.dll
  dllEntry
  vmMain

etmain\cgame_mp_x86.dll
  dllEntry
  vmMain

etmain\ui_mp_x86.dll
  dllEntry
  vmMain
```

Notes:

```text
Classic Id Tech 3 VM DLL layout. The project now includes Win32 build configurations for this generation.
```

## Quake II RTX

```text
baseq2\gamex86.dll
  GetGameAPI

baseq2\gamex86_64.dll
  GetGameAPI
```

Notes:

```text
Quake II-style game DLL API. The x64 build is the expected fit for q2rtx.exe and gamex86_64.dll.
```

## FVI / Warfork

```text
libs\ref_gl_x64.dll
  GetRefAPI

libs\ui_x64.dll
  GetUIAPI

libs\angelwrap_x64.dll
  GetAngelwrapAPI

libs\ftlib_x64.dll
  GetFTLibAPI

warfork_x64.exe
  AmdPowerXpressRequestHighPerformance
  NvOptimusEnablement
```

Notes:

```text
The client executable only exports GPU preference symbols, but the loaded module set has strong Warfork/FVI API evidence in renderer, UI, scripting, and text modules.
```
