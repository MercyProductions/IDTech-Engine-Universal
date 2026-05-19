#pragma once

#include "AegisUniversalRuntime.h"

inline constexpr AegisUniversalSignature kAegisUniversalSignatures[] = {
    { L"doomx64.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { L"doometernalx64vk.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { L"et.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Wolfenstein Enemy Territory client process" },
    { L"etded.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Wolfenstein Enemy Territory dedicated server process" },
    { L"quake4.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { L"quake2.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { L"q2rtx.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Quake II RTX client process" },
    { L"q2rtxded.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Quake II RTX dedicated server process" },
    { L"rage2.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { L"warfork_x64.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "FVI/Warfork id Tech lineage client process" },
    { L"wf_server_x64.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "FVI/Warfork dedicated server process" },
    { L"wftv_server_x64.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "FVI/Warfork TV server process" },
    { L"wolfenstein", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { nullptr, L"angelwrap_x64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "FVI/Warfork AngelScript bridge module" },
    { nullptr, L"cgame_mp_x86.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "id Tech 3 client game VM DLL" },
    { nullptr, L"ftlib_x64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "FVI/Warfork font/text utility module" },
    { nullptr, L"gamex86_64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Quake II RTX 64-bit game DLL" },
    { nullptr, L"idtech", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, L"gamex86.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, L"gamex64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, L"qagame_mp_x86.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "id Tech 3 server game VM DLL" },
    { nullptr, L"ref_gl_x64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "FVI/Warfork OpenGL renderer module" },
    { nullptr, L"renderprogs", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Module hint" },
    { nullptr, L"ui_mp_x86.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "id Tech 3 UI VM DLL" },
    { nullptr, L"ui_x64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "FVI/Warfork UI module" },
    { nullptr, L"vulkan-1.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Module hint" },
    { nullptr, L"bink2w64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, nullptr, "GetAngelwrapAPI", AegisUniversalSignature_Export | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "FVI/Warfork AngelScript API export" },
    { nullptr, nullptr, "GetFTLibAPI", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "FVI/Warfork text/font API export" },
    { nullptr, nullptr, "GetGameAPI", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "Export hint" },
    { nullptr, nullptr, "GetModuleAPI", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "Export hint" },
    { nullptr, nullptr, "GetRefAPI", AegisUniversalSignature_Export | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "FVI/Warfork renderer API export" },
    { nullptr, nullptr, "GetUIAPI", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "FVI/Warfork UI API export" },
    { nullptr, nullptr, "vmMain", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "Export hint" },
    { nullptr, nullptr, "dllEntry", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "Export hint" },
};

inline constexpr AegisUniversalProfile kAegisUniversalProfile = {
    L"id Tech",
    L"idTech",
    L"idTech_Universal_Report.txt",
    L"idTech_Universal_Trace.txt",
    kAegisUniversalSignatures,
    sizeof(kAegisUniversalSignatures) / sizeof(kAegisUniversalSignatures[0])
};

