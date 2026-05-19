#pragma once

#include "AegisUniversalRuntime.h"

#if defined(_MSC_VER)
#define AEGIS_IDTECH_CALL __stdcall
#else
#define AEGIS_IDTECH_CALL
#endif

enum AegisIdTechMatrixFlags : std::uint32_t
{
    AegisIdTechMatrix_Auto = 0,
    AegisIdTechMatrix_RowMajor = 1u << 0,
    AegisIdTechMatrix_ColumnMajor = 1u << 1,
    AegisIdTechMatrix_D3DDepth = 1u << 2,
    AegisIdTechMatrix_OpenGLDepth = 1u << 3,
    AegisIdTechMatrix_YFlip = 1u << 4
};

struct AegisIdTechVec3
{
    float x;
    float y;
    float z;
};

struct AegisIdTechViewport
{
    float x;
    float y;
    float width;
    float height;
};

struct AegisIdTechMatrix4x4
{
    float m[16];
    std::uint32_t flags;
};

struct AegisIdTechEntitySnapshot
{
    std::uint64_t id;
    char name[96];
    char className[64];
    AegisIdTechVec3 origin;
    AegisIdTechVec3 boundsMin;
    AegisIdTechVec3 boundsMax;
    std::int32_t team;
    std::int32_t visible;
    std::uint32_t flags;
};

struct AegisIdTechProjectedPoint
{
    float x;
    float y;
    float depth;
    std::int32_t clipped;
};

struct AegisIdTechAdapterTiming
{
    double entityProviderMs;
    double matrixProviderMs;
    double viewportProviderMs;
    std::uint32_t entityCount;
    std::uint32_t projectedCount;
    std::uint32_t clippedCount;
    std::uint64_t frameId;
};

struct AegisIdTechCapabilityInfo
{
    std::int32_t idTechDetected;
    std::int32_t gameApiFound;
    std::int32_t vmApiFound;
    std::int32_t rendererApiFound;
    std::int32_t scriptingApiFound;
    std::int32_t entityProviderRegistered;
    std::int32_t viewProjectionProviderRegistered;
    std::int32_t viewportProviderRegistered;
    std::int32_t viewportValid;
    std::int32_t matrixValid;
    std::int32_t w2sProjectionWorking;
    std::int32_t snapshotReady;
    wchar_t rendererBackend[32];
    wchar_t details[256];
};

using AegisIdTechEntityProvider = std::uint32_t(AEGIS_IDTECH_CALL*)(
    AegisIdTechEntitySnapshot* outEntities,
    std::uint32_t capacity,
    void* userData);

using AegisIdTechViewProjectionProvider = std::int32_t(AEGIS_IDTECH_CALL*)(
    AegisIdTechMatrix4x4* outMatrix,
    void* userData);

using AegisIdTechViewportProvider = std::int32_t(AEGIS_IDTECH_CALL*)(
    AegisIdTechViewport* outViewport,
    void* userData);

AEGIS_UNIVERSAL_API void AegisIdTech_RegisterEntityProvider(AegisIdTechEntityProvider provider, void* userData);
AEGIS_UNIVERSAL_API void AegisIdTech_RegisterViewProjectionProvider(AegisIdTechViewProjectionProvider provider, void* userData);
AEGIS_UNIVERSAL_API void AegisIdTech_RegisterViewportProvider(AegisIdTechViewportProvider provider, void* userData);
AEGIS_UNIVERSAL_API int AegisIdTech_UpdateProviders();
AEGIS_UNIVERSAL_API int AegisIdTech_SubmitEntitySnapshots(const AegisIdTechEntitySnapshot* entities, std::uint32_t count);
AEGIS_UNIVERSAL_API int AegisIdTech_SubmitViewProjection(const AegisIdTechMatrix4x4* matrix);
AEGIS_UNIVERSAL_API int AegisIdTech_SubmitViewport(const AegisIdTechViewport* viewport);
AEGIS_UNIVERSAL_API std::uint32_t AegisIdTech_GetEntityCount();
AEGIS_UNIVERSAL_API int AegisIdTech_GetEntitySnapshot(std::uint32_t index, AegisIdTechEntitySnapshot* outEntity);
AEGIS_UNIVERSAL_API int AegisIdTech_GetAdapterTiming(AegisIdTechAdapterTiming* outTiming);
AEGIS_UNIVERSAL_API int AegisIdTech_GetCapabilityInfo(AegisIdTechCapabilityInfo* outInfo);
AEGIS_UNIVERSAL_API int AegisIdTech_ProjectWorldToScreen(const AegisIdTechVec3* world, AegisIdTechProjectedPoint* outPoint);
AEGIS_UNIVERSAL_API int AegisIdTech_WriteSnapshotJson(const wchar_t* path);
AEGIS_UNIVERSAL_API int AegisIdTech_LoadSnapshotJson(const wchar_t* path);
AEGIS_UNIVERSAL_API void AegisIdTech_PrintCurrentEntities();
