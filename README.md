# id Tech Universal

Aegis universal runtime project for **id Tech** lineage targets. This project follows the CryUniversal diagnostic-SDK model: identify engine/runtime capabilities, expose a provider-based entity and camera adapter, validate world-to-screen projection, and write clean reports/snapshots from submitted debug data.

It does not add game-specific memory scanners, actor offset scraping, anti-cheat bypasses, stealth behavior, or hidden injection behavior.

## Dumpbin Findings

The profile was refreshed from these local targets:

```text
C:\Program Files (x86)\Steam\steamapps\common\fvi
C:\Program Files (x86)\Steam\steamapps\common\Wolfenstein Enemy Territory
C:\Program Files (x86)\Steam\steamapps\common\Quake II RTX
```

Confirmed exports:

```text
Wolfenstein Enemy Territory\etmain\qagame_mp_x86.dll  -> dllEntry, vmMain
Wolfenstein Enemy Territory\etmain\cgame_mp_x86.dll   -> dllEntry, vmMain
Wolfenstein Enemy Territory\etmain\ui_mp_x86.dll      -> dllEntry, vmMain
Quake II RTX\baseq2\gamex86.dll                       -> GetGameAPI
Quake II RTX\baseq2\gamex86_64.dll                    -> GetGameAPI
fvi\libs\ref_gl_x64.dll                               -> GetRefAPI
fvi\libs\ui_x64.dll                                   -> GetUIAPI
fvi\libs\angelwrap_x64.dll                            -> GetAngelwrapAPI
fvi\libs\ftlib_x64.dll                                -> GetFTLibAPI
```

## Runtime Flow

1. Load the DLL into a process you are authorized to inspect.
2. The bootstrap thread initializes the shared Aegis runtime.
3. Loaded modules are enumerated and matched against Id Tech process/module/export hints.
4. The Id Tech adapter can receive entity snapshots, viewport size, and view-projection matrices from a debug/test provider.
5. The adapter validates matrix/viewport health, projects submitted world points to screen coordinates, records JSON snapshots, and reports backend/capability status.

## Adapter API

The Id Tech-specific API is declared in:

```text
Include\AegisIdTechUniversal.h
```

Core provider functions:

```cpp
AegisIdTech_RegisterEntityProvider(...);
AegisIdTech_RegisterViewProjectionProvider(...);
AegisIdTech_RegisterViewportProvider(...);
AegisIdTech_UpdateProviders();
```

Direct submission functions:

```cpp
AegisIdTech_SubmitEntitySnapshots(...);
AegisIdTech_SubmitViewProjection(...);
AegisIdTech_SubmitViewport(...);
```

Diagnostics:

```cpp
AegisIdTech_GetCapabilityInfo(...);
AegisIdTech_ProjectWorldToScreen(...);
AegisIdTech_WriteSnapshotJson(...);
AegisIdTech_LoadSnapshotJson(...);
AegisIdTech_PrintCurrentEntities();
```

## Entity Snapshot

Each submitted entity snapshot contains:

```text
id
name
className
origin
boundsMin / boundsMax
team
visible
flags
```

This lets an Id Tech debug adapter submit players, NPCs, items, movers, or other engine objects without hardcoding title-specific offsets into the universal DLL.

## Renderer Status

`AegisIdTech_GetCapabilityInfo` reports the detected backend as one of:

```text
Vulkan
OpenGL
Direct3D12
Direct3D11
Unknown
```

The backend is inferred from currently loaded modules such as `vulkan-1.dll`, `opengl32.dll`, `ref_gl_x64.dll`, `d3d11.dll`, `d3d12.dll`, and `dxgi.dll`.

## Internal ImGui Overlay

The DLL now owns an in-process ImGui diagnostics menu. On load it starts an internal render bridge and attempts backend setup in this order:

```text
Direct3D11 Present
Direct3D9 EndScene
OpenGL SwapBuffers
```

Press `F4` to show or hide the menu. The menu includes runtime status, renderer status, adapter capability checks, entity/provider timing, and overlay toggles for boxes, corner boxes, filled boxes, lines, and labels. Vulkan and D3D12 are still detected and reported clearly, but this build does not render ImGui through those backends yet.

## Build

This repository vendors the small shared Aegis runtime under:

```text
Common\AegisUniversalRuntime
```

x64:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" ".\AegisIdTechUniversal.sln" /m /p:Configuration=Release /p:Platform=x64 /v:minimal
```

Win32, useful for classic Id Tech 3-era targets such as Wolfenstein Enemy Territory:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" ".\AegisIdTechUniversal.sln" /m /p:Configuration=Release /p:Platform=Win32 /v:minimal
```

Outputs:

```text
build\x64\Release\AegisIdTechUniversal.dll
build\Win32\Release\AegisIdTechUniversal.dll
```

## Sample Harness

A local provider harness is included at:

```text
Samples\IdTechProviderHarness\IdTechProviderHarness.cpp
```

It loads the DLL, registers known test entities, submits a viewport and matrix, prints entity names, and writes `IdTechProviderHarness.snapshot.json` so W2S and replay can be tested without relying on a specific game.
