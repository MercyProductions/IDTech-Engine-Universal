#include "../../Include/AegisIdTechUniversal.h"

#include <Windows.h>

#include <cstdio>
#include <cwchar>
#include <cstring>

namespace
{
    std::uint32_t AEGIS_IDTECH_CALL SampleEntities(AegisIdTechEntitySnapshot* outEntities, std::uint32_t capacity, void*)
    {
        if (!outEntities || capacity < 3)
            return 0;

        AegisIdTechEntitySnapshot samples[3] = {};
        samples[0].id = 1;
        strcpy_s(samples[0].name, "idtech_test_player");
        strcpy_s(samples[0].className, "player");
        samples[0].origin = { 0.0f, 0.0f, 4.0f };
        samples[0].boundsMin = { -16.0f, -16.0f, 0.0f };
        samples[0].boundsMax = { 16.0f, 16.0f, 56.0f };
        samples[0].team = 1;
        samples[0].visible = 1;

        samples[1].id = 2;
        strcpy_s(samples[1].name, "idtech_test_npc");
        strcpy_s(samples[1].className, "npc");
        samples[1].origin = { 1.5f, 0.0f, 6.0f };
        samples[1].boundsMin = { -12.0f, -12.0f, 0.0f };
        samples[1].boundsMax = { 12.0f, 12.0f, 48.0f };
        samples[1].team = 2;
        samples[1].visible = 1;

        samples[2].id = 3;
        strcpy_s(samples[2].name, "idtech_test_pickup");
        strcpy_s(samples[2].className, "item");
        samples[2].origin = { -1.5f, 0.0f, 5.0f };
        samples[2].boundsMin = { -8.0f, -8.0f, -8.0f };
        samples[2].boundsMax = { 8.0f, 8.0f, 8.0f };
        samples[2].team = 0;
        samples[2].visible = 1;

        std::memcpy(outEntities, samples, sizeof(samples));
        return 3;
    }

    std::int32_t AEGIS_IDTECH_CALL SampleViewport(AegisIdTechViewport* outViewport, void*)
    {
        if (!outViewport)
            return 0;
        *outViewport = { 0.0f, 0.0f, 1280.0f, 720.0f };
        return 1;
    }

    std::int32_t AEGIS_IDTECH_CALL SampleMatrix(AegisIdTechMatrix4x4* outMatrix, void*)
    {
        if (!outMatrix)
            return 0;

        *outMatrix = {};
        outMatrix->flags = AegisIdTechMatrix_RowMajor | AegisIdTechMatrix_OpenGLDepth;
        outMatrix->m[0] = 1.0f;
        outMatrix->m[5] = 1.0f;
        outMatrix->m[10] = 1.0f;
        outMatrix->m[14] = 1.0f;
        return 1;
    }

    template <typename T>
    bool LoadFunction(HMODULE module, const char* name, T& outFunction)
    {
        outFunction = reinterpret_cast<T>(::GetProcAddress(module, name));
        if (!outFunction)
            std::printf("missing export: %s\n", name);
        return outFunction != nullptr;
    }
}

int wmain(int argc, wchar_t** argv)
{
    const wchar_t* dllPath = argc > 1 ? argv[1] : L"..\\..\\build\\x64\\Release\\AegisIdTechUniversal.dll";
    HMODULE dll = ::LoadLibraryW(dllPath);
    if (!dll)
    {
        ::wprintf(L"LoadLibrary failed: %ls (%lu)\n", dllPath, ::GetLastError());
        return 1;
    }

    using RegisterEntitiesFn = void(__cdecl*)(AegisIdTechEntityProvider, void*);
    using RegisterMatrixFn = void(__cdecl*)(AegisIdTechViewProjectionProvider, void*);
    using RegisterViewportFn = void(__cdecl*)(AegisIdTechViewportProvider, void*);
    using UpdateFn = int(__cdecl*)();
    using CapabilityFn = int(__cdecl*)(AegisIdTechCapabilityInfo*);
    using CountFn = std::uint32_t(__cdecl*)();
    using WriteJsonFn = int(__cdecl*)(const wchar_t*);
    using PrintFn = void(__cdecl*)();

    RegisterEntitiesFn registerEntities = nullptr;
    RegisterMatrixFn registerMatrix = nullptr;
    RegisterViewportFn registerViewport = nullptr;
    UpdateFn update = nullptr;
    CapabilityFn getCapability = nullptr;
    CountFn getCount = nullptr;
    WriteJsonFn writeJson = nullptr;
    PrintFn printEntities = nullptr;

    if (!LoadFunction(dll, "AegisIdTech_RegisterEntityProvider", registerEntities) ||
        !LoadFunction(dll, "AegisIdTech_RegisterViewProjectionProvider", registerMatrix) ||
        !LoadFunction(dll, "AegisIdTech_RegisterViewportProvider", registerViewport) ||
        !LoadFunction(dll, "AegisIdTech_UpdateProviders", update) ||
        !LoadFunction(dll, "AegisIdTech_GetCapabilityInfo", getCapability) ||
        !LoadFunction(dll, "AegisIdTech_GetEntityCount", getCount) ||
        !LoadFunction(dll, "AegisIdTech_WriteSnapshotJson", writeJson) ||
        !LoadFunction(dll, "AegisIdTech_PrintCurrentEntities", printEntities))
    {
        return 2;
    }

    registerEntities(SampleEntities, nullptr);
    registerMatrix(SampleMatrix, nullptr);
    registerViewport(SampleViewport, nullptr);
    update();

    AegisIdTechCapabilityInfo capability = {};
    getCapability(&capability);
    ::wprintf(L"backend=%ls details=%ls\n", capability.rendererBackend, capability.details);
    std::printf("entities=%u\n", getCount());
    printEntities();
    writeJson(L"IdTechProviderHarness.snapshot.json");
    return 0;
}
