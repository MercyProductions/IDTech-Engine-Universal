#include "AegisIdTechUniversal.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr std::uint32_t kMaxEntities = 2048;

    struct ProviderState
    {
        AegisIdTechEntityProvider entityProvider = nullptr;
        void* entityUserData = nullptr;
        AegisIdTechViewProjectionProvider matrixProvider = nullptr;
        void* matrixUserData = nullptr;
        AegisIdTechViewportProvider viewportProvider = nullptr;
        void* viewportUserData = nullptr;
    };

    std::mutex g_adapterMutex;
    ProviderState g_providers;
    std::vector<AegisIdTechEntitySnapshot> g_entities;
    AegisIdTechMatrix4x4 g_viewProjection = {};
    AegisIdTechViewport g_viewport = {};
    AegisIdTechAdapterTiming g_timing = {};
    bool g_hasMatrix = false;
    bool g_hasViewport = false;

    template <std::size_t N>
    void CopyWide(wchar_t (&dest)[N], const wchar_t* value)
    {
        wcsncpy_s(dest, value ? value : L"", _TRUNCATE);
    }

    template <std::size_t N>
    void CopyAnsi(char (&dest)[N], const std::string& value)
    {
        strncpy_s(dest, value.c_str(), _TRUNCATE);
    }

    bool IsFinite(float value)
    {
        return std::isfinite(value);
    }

    bool IsValidViewport(const AegisIdTechViewport& viewport)
    {
        return IsFinite(viewport.x) && IsFinite(viewport.y) &&
            IsFinite(viewport.width) && IsFinite(viewport.height) &&
            viewport.width > 1.0f && viewport.height > 1.0f;
    }

    bool IsValidMatrix(const AegisIdTechMatrix4x4& matrix)
    {
        bool anyNonZero = false;
        for (float value : matrix.m)
        {
            if (!IsFinite(value))
                return false;
            anyNonZero = anyNonZero || std::fabs(value) > 0.000001f;
        }
        return anyNonZero;
    }

    LARGE_INTEGER NowCounter()
    {
        LARGE_INTEGER value = {};
        ::QueryPerformanceCounter(&value);
        return value;
    }

    double ElapsedMs(LARGE_INTEGER start, LARGE_INTEGER end)
    {
        LARGE_INTEGER frequency = {};
        ::QueryPerformanceFrequency(&frequency);
        if (frequency.QuadPart == 0)
            return 0.0;
        return (static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0) /
            static_cast<double>(frequency.QuadPart);
    }

    std::wstring Lower(std::wstring value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
            return static_cast<wchar_t>(::towlower(ch));
        });
        return value;
    }

    bool Contains(const std::wstring& value, const wchar_t* needle)
    {
        return needle && Lower(value).find(Lower(needle)) != std::wstring::npos;
    }

    bool ContainsAnsi(const char* value, const char* needle)
    {
        if (!value || !needle)
            return false;

        std::string haystack = value;
        std::string target = needle;
        std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        std::transform(target.begin(), target.end(), target.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return haystack.find(target) != std::string::npos;
    }

    bool ProjectWithConvention(
        const AegisIdTechMatrix4x4& matrix,
        const AegisIdTechViewport& viewport,
        const AegisIdTechVec3& world,
        bool columnMajor,
        bool openGlDepth,
        bool yFlip,
        AegisIdTechProjectedPoint& outPoint)
    {
        const float* m = matrix.m;
        float clipX = 0.0f;
        float clipY = 0.0f;
        float clipZ = 0.0f;
        float clipW = 0.0f;

        if (columnMajor)
        {
            clipX = world.x * m[0] + world.y * m[4] + world.z * m[8] + m[12];
            clipY = world.x * m[1] + world.y * m[5] + world.z * m[9] + m[13];
            clipZ = world.x * m[2] + world.y * m[6] + world.z * m[10] + m[14];
            clipW = world.x * m[3] + world.y * m[7] + world.z * m[11] + m[15];
        }
        else
        {
            clipX = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
            clipY = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
            clipZ = world.x * m[8] + world.y * m[9] + world.z * m[10] + m[11];
            clipW = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];
        }

        if (!IsFinite(clipX) || !IsFinite(clipY) || !IsFinite(clipZ) || !IsFinite(clipW) || std::fabs(clipW) < 0.00001f)
            return false;

        const float ndcX = clipX / clipW;
        const float ndcY = clipY / clipW;
        const float ndcZ = clipZ / clipW;
        if (!IsFinite(ndcX) || !IsFinite(ndcY) || !IsFinite(ndcZ))
            return false;

        const float screenX = viewport.x + ((ndcX * 0.5f) + 0.5f) * viewport.width;
        const float screenY = viewport.y + (yFlip ? ((ndcY * 0.5f) + 0.5f) : (0.5f - (ndcY * 0.5f))) * viewport.height;
        const bool depthClipped = openGlDepth ? (ndcZ < -1.0f || ndcZ > 1.0f) : (ndcZ < 0.0f || ndcZ > 1.0f);
        const bool xyClipped = ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f;

        outPoint.x = screenX;
        outPoint.y = screenY;
        outPoint.depth = ndcZ;
        outPoint.clipped = (xyClipped || depthClipped || clipW < 0.0f) ? 1 : 0;
        return IsFinite(screenX) && IsFinite(screenY);
    }

    bool ProjectCurrentLocked(const AegisIdTechVec3& world, AegisIdTechProjectedPoint& outPoint)
    {
        if (!g_hasMatrix || !g_hasViewport || !IsValidMatrix(g_viewProjection) || !IsValidViewport(g_viewport))
            return false;

        const std::uint32_t flags = g_viewProjection.flags;
        const bool wantsColumn = (flags & AegisIdTechMatrix_ColumnMajor) != 0;
        const bool wantsRow = (flags & AegisIdTechMatrix_RowMajor) != 0;
        const bool openGlDepth = (flags & AegisIdTechMatrix_OpenGLDepth) != 0 || (flags & AegisIdTechMatrix_D3DDepth) == 0;
        const bool yFlip = (flags & AegisIdTechMatrix_YFlip) != 0;

        if (wantsColumn || wantsRow)
            return ProjectWithConvention(g_viewProjection, g_viewport, world, wantsColumn, openGlDepth, yFlip, outPoint);

        AegisIdTechProjectedPoint row = {};
        if (ProjectWithConvention(g_viewProjection, g_viewport, world, false, openGlDepth, yFlip, row) && !row.clipped)
        {
            outPoint = row;
            return true;
        }

        AegisIdTechProjectedPoint column = {};
        if (ProjectWithConvention(g_viewProjection, g_viewport, world, true, openGlDepth, yFlip, column))
        {
            outPoint = column;
            return true;
        }

        if (ProjectWithConvention(g_viewProjection, g_viewport, world, false, openGlDepth, yFlip, row))
        {
            outPoint = row;
            return true;
        }
        return false;
    }

    void RebuildProjectionStatsLocked()
    {
        g_timing.entityCount = static_cast<std::uint32_t>(g_entities.size());
        g_timing.projectedCount = 0;
        g_timing.clippedCount = 0;

        for (const AegisIdTechEntitySnapshot& entity : g_entities)
        {
            AegisIdTechProjectedPoint point = {};
            if (!ProjectCurrentLocked(entity.origin, point))
                continue;
            if (point.clipped)
                ++g_timing.clippedCount;
            else
                ++g_timing.projectedCount;
        }
    }

    std::string JsonEscape(const char* value)
    {
        std::string escaped;
        if (!value)
            return escaped;

        for (const char* cursor = value; *cursor; ++cursor)
        {
            switch (*cursor)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(*cursor);
                break;
            }
        }
        return escaped;
    }

    std::string ExtractJsonString(const std::string& object, const char* key)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return {};
        pos = object.find(':', pos);
        pos = object.find('"', pos);
        if (pos == std::string::npos)
            return {};
        ++pos;

        std::string result;
        for (; pos < object.size(); ++pos)
        {
            const char ch = object[pos];
            if (ch == '\\' && pos + 1 < object.size())
            {
                result.push_back(object[pos + 1]);
                ++pos;
                continue;
            }
            if (ch == '"')
                break;
            result.push_back(ch);
        }
        return result;
    }

    bool ExtractJsonNumber(const std::string& object, const char* key, float& outValue)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return false;
        pos = object.find(':', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        const float value = std::strtof(object.c_str() + pos + 1, &end);
        if (end == object.c_str() + pos + 1 || !IsFinite(value))
            return false;
        outValue = value;
        return true;
    }

    bool ExtractJsonUInt64(const std::string& object, const char* key, std::uint64_t& outValue)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return false;
        pos = object.find(':', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        const unsigned long long value = std::strtoull(object.c_str() + pos + 1, &end, 10);
        if (end == object.c_str() + pos + 1)
            return false;
        outValue = static_cast<std::uint64_t>(value);
        return true;
    }

    bool ExtractJsonInt(const std::string& object, const char* key, std::int32_t& outValue)
    {
        float value = 0.0f;
        if (!ExtractJsonNumber(object, key, value))
            return false;
        outValue = static_cast<std::int32_t>(value);
        return true;
    }

    bool ExtractJsonVec3(const std::string& object, const char* key, AegisIdTechVec3& outValue)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return false;
        pos = object.find('[', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        outValue.x = std::strtof(object.c_str() + pos + 1, &end);
        if (!end || *end == '\0')
            return false;
        outValue.y = std::strtof(end + 1, &end);
        if (!end || *end == '\0')
            return false;
        outValue.z = std::strtof(end + 1, &end);
        return IsFinite(outValue.x) && IsFinite(outValue.y) && IsFinite(outValue.z);
    }

    bool ExtractJsonMatrix(const std::string& json, AegisIdTechMatrix4x4& outMatrix)
    {
        std::size_t pos = json.find("\"m\"");
        if (pos == std::string::npos)
            return false;
        pos = json.find('[', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        const char* cursor = json.c_str() + pos + 1;
        for (float& value : outMatrix.m)
        {
            value = std::strtof(cursor, &end);
            if (end == cursor || !IsFinite(value))
                return false;
            cursor = end + 1;
        }

        float flags = 0.0f;
        if (ExtractJsonNumber(json, "matrixFlags", flags))
            outMatrix.flags = static_cast<std::uint32_t>(flags);
        return true;
    }

    std::wstring DetectRendererBackend()
    {
        if (!AegisUniversal_IsInitialized())
            AegisUniversal_Initialize();

        bool hasVulkan = false;
        bool hasOpenGl = false;
        bool hasD3D11 = false;
        bool hasD3D12 = false;
        const std::uint32_t count = AegisUniversal_GetModuleCount();
        for (std::uint32_t index = 0; index < count; ++index)
        {
            AegisUniversalModuleInfo module = {};
            if (!AegisUniversal_GetModuleInfo(index, &module))
                continue;
            const std::wstring name = module.name;
            hasVulkan = hasVulkan || Contains(name, L"vulkan") || Contains(name, L"q2rtx");
            hasOpenGl = hasOpenGl || Contains(name, L"opengl32") || Contains(name, L"ref_gl");
            hasD3D11 = hasD3D11 || Contains(name, L"d3d11") || Contains(name, L"dxgi");
            hasD3D12 = hasD3D12 || Contains(name, L"d3d12");
        }

        if (hasVulkan)
            return L"Vulkan";
        if (hasOpenGl)
            return L"OpenGL";
        if (hasD3D12)
            return L"Direct3D12";
        if (hasD3D11)
            return L"Direct3D11";
        return L"Unknown";
    }
}

AEGIS_UNIVERSAL_API void AegisIdTech_RegisterEntityProvider(AegisIdTechEntityProvider provider, void* userData)
{
    std::lock_guard lock(g_adapterMutex);
    g_providers.entityProvider = provider;
    g_providers.entityUserData = userData;
}

AEGIS_UNIVERSAL_API void AegisIdTech_RegisterViewProjectionProvider(AegisIdTechViewProjectionProvider provider, void* userData)
{
    std::lock_guard lock(g_adapterMutex);
    g_providers.matrixProvider = provider;
    g_providers.matrixUserData = userData;
}

AEGIS_UNIVERSAL_API void AegisIdTech_RegisterViewportProvider(AegisIdTechViewportProvider provider, void* userData)
{
    std::lock_guard lock(g_adapterMutex);
    g_providers.viewportProvider = provider;
    g_providers.viewportUserData = userData;
}

AEGIS_UNIVERSAL_API int AegisIdTech_UpdateProviders()
{
    ProviderState providers = {};
    {
        std::lock_guard lock(g_adapterMutex);
        providers = g_providers;
    }

    std::vector<AegisIdTechEntitySnapshot> entities(kMaxEntities);
    std::uint32_t entityCount = 0;
    double entityMs = 0.0;
    double matrixMs = 0.0;
    double viewportMs = 0.0;
    bool hasMatrix = false;
    bool hasViewport = false;
    AegisIdTechMatrix4x4 matrix = {};
    AegisIdTechViewport viewport = {};

    if (providers.viewportProvider)
    {
        const LARGE_INTEGER start = NowCounter();
        hasViewport = providers.viewportProvider(&viewport, providers.viewportUserData) != 0 && IsValidViewport(viewport);
        viewportMs = ElapsedMs(start, NowCounter());
    }

    if (providers.matrixProvider)
    {
        const LARGE_INTEGER start = NowCounter();
        hasMatrix = providers.matrixProvider(&matrix, providers.matrixUserData) != 0 && IsValidMatrix(matrix);
        matrixMs = ElapsedMs(start, NowCounter());
    }

    if (providers.entityProvider)
    {
        const LARGE_INTEGER start = NowCounter();
        entityCount = providers.entityProvider(entities.data(), kMaxEntities, providers.entityUserData);
        entityMs = ElapsedMs(start, NowCounter());
        entityCount = std::min<std::uint32_t>(entityCount, kMaxEntities);
        entities.resize(entityCount);
    }
    else
    {
        entities.clear();
    }

    std::lock_guard lock(g_adapterMutex);
    if (providers.entityProvider)
        g_entities = std::move(entities);
    if (providers.matrixProvider)
    {
        g_viewProjection = matrix;
        g_hasMatrix = hasMatrix;
    }
    if (providers.viewportProvider)
    {
        g_viewport = viewport;
        g_hasViewport = hasViewport;
    }
    g_timing.entityProviderMs = entityMs;
    g_timing.matrixProviderMs = matrixMs;
    g_timing.viewportProviderMs = viewportMs;
    ++g_timing.frameId;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API int AegisIdTech_SubmitEntitySnapshots(const AegisIdTechEntitySnapshot* entities, std::uint32_t count)
{
    if (!entities && count != 0)
        return 0;

    const std::uint32_t clampedCount = std::min<std::uint32_t>(count, kMaxEntities);
    std::lock_guard lock(g_adapterMutex);
    g_entities.assign(entities, entities + clampedCount);
    ++g_timing.frameId;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API int AegisIdTech_SubmitViewProjection(const AegisIdTechMatrix4x4* matrix)
{
    if (!matrix || !IsValidMatrix(*matrix))
        return 0;

    std::lock_guard lock(g_adapterMutex);
    g_viewProjection = *matrix;
    g_hasMatrix = true;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API int AegisIdTech_SubmitViewport(const AegisIdTechViewport* viewport)
{
    if (!viewport || !IsValidViewport(*viewport))
        return 0;

    std::lock_guard lock(g_adapterMutex);
    g_viewport = *viewport;
    g_hasViewport = true;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API std::uint32_t AegisIdTech_GetEntityCount()
{
    std::lock_guard lock(g_adapterMutex);
    return static_cast<std::uint32_t>(g_entities.size());
}

AEGIS_UNIVERSAL_API int AegisIdTech_GetEntitySnapshot(std::uint32_t index, AegisIdTechEntitySnapshot* outEntity)
{
    if (!outEntity)
        return 0;

    std::lock_guard lock(g_adapterMutex);
    if (index >= g_entities.size())
        return 0;
    *outEntity = g_entities[index];
    return 1;
}

AEGIS_UNIVERSAL_API int AegisIdTech_GetAdapterTiming(AegisIdTechAdapterTiming* outTiming)
{
    if (!outTiming)
        return 0;
    std::lock_guard lock(g_adapterMutex);
    *outTiming = g_timing;
    return 1;
}

AEGIS_UNIVERSAL_API int AegisIdTech_GetCapabilityInfo(AegisIdTechCapabilityInfo* outInfo)
{
    if (!outInfo)
        return 0;

    if (!AegisUniversal_IsInitialized())
        AegisUniversal_Initialize();

    AegisUniversalRuntimeInfo runtime = {};
    AegisUniversal_GetRuntimeInfo(&runtime);

    bool gameApi = false;
    bool vmApi = false;
    bool rendererApi = false;
    bool scriptingApi = false;
    const std::uint32_t exportCount = AegisUniversal_GetMatchedExportCount();
    for (std::uint32_t index = 0; index < exportCount; ++index)
    {
        AegisUniversalExportInfo exportInfo = {};
        if (!AegisUniversal_GetMatchedExportInfo(index, &exportInfo))
            continue;

        gameApi = gameApi || ContainsAnsi(exportInfo.exportName, "GetGameAPI");
        vmApi = vmApi || ContainsAnsi(exportInfo.exportName, "vmMain") || ContainsAnsi(exportInfo.exportName, "dllEntry");
        rendererApi = rendererApi || ContainsAnsi(exportInfo.exportName, "GetRefAPI");
        scriptingApi = scriptingApi || ContainsAnsi(exportInfo.exportName, "GetAngelwrapAPI");
    }

    std::lock_guard lock(g_adapterMutex);
    *outInfo = {};
    outInfo->idTechDetected = (runtime.flags & AegisUniversalRuntime_EngineDetected) ? 1 : 0;
    outInfo->gameApiFound = gameApi ? 1 : 0;
    outInfo->vmApiFound = vmApi ? 1 : 0;
    outInfo->rendererApiFound = rendererApi ? 1 : 0;
    outInfo->scriptingApiFound = scriptingApi ? 1 : 0;
    outInfo->entityProviderRegistered = g_providers.entityProvider ? 1 : 0;
    outInfo->viewProjectionProviderRegistered = g_providers.matrixProvider ? 1 : 0;
    outInfo->viewportProviderRegistered = g_providers.viewportProvider ? 1 : 0;
    outInfo->viewportValid = (g_hasViewport && IsValidViewport(g_viewport)) ? 1 : 0;
    outInfo->matrixValid = (g_hasMatrix && IsValidMatrix(g_viewProjection)) ? 1 : 0;
    outInfo->snapshotReady = !g_entities.empty() ? 1 : 0;

    AegisIdTechProjectedPoint point = {};
    const AegisIdTechVec3 origin = { 0.0f, 0.0f, 0.0f };
    outInfo->w2sProjectionWorking = ProjectCurrentLocked(origin, point) ? 1 : 0;
    CopyWide(outInfo->rendererBackend, DetectRendererBackend().c_str());

    std::wstringstream details;
    details << L"modules " << runtime.moduleCount
            << L", matched exports " << runtime.matchedExportCount
            << L", entities " << g_entities.size()
            << L", projected " << g_timing.projectedCount
            << L", clipped " << g_timing.clippedCount;
    CopyWide(outInfo->details, details.str().c_str());
    return 1;
}

AEGIS_UNIVERSAL_API int AegisIdTech_ProjectWorldToScreen(const AegisIdTechVec3* world, AegisIdTechProjectedPoint* outPoint)
{
    if (!world || !outPoint)
        return 0;

    std::lock_guard lock(g_adapterMutex);
    *outPoint = {};
    return ProjectCurrentLocked(*world, *outPoint) ? 1 : 0;
}

AEGIS_UNIVERSAL_API int AegisIdTech_WriteSnapshotJson(const wchar_t* path)
{
    if (!path || !path[0])
        return 0;

    std::lock_guard lock(g_adapterMutex);
    std::wofstream out(std::filesystem::path(path), std::ios::trunc);
    if (!out)
        return 0;

    out << L"{\n";
    out << L"  \"engine\": \"id Tech\",\n";
    out << L"  \"frameId\": " << g_timing.frameId << L",\n";
    out << L"  \"viewport\": { \"x\": " << g_viewport.x << L", \"y\": " << g_viewport.y
        << L", \"width\": " << g_viewport.width << L", \"height\": " << g_viewport.height << L" },\n";
    out << L"  \"matrixFlags\": " << g_viewProjection.flags << L",\n";
    out << L"  \"m\": [";
    for (std::size_t index = 0; index < 16; ++index)
    {
        if (index)
            out << L", ";
        out << g_viewProjection.m[index];
    }
    out << L"],\n";
    out << L"  \"timing\": { \"entityProviderMs\": " << g_timing.entityProviderMs
        << L", \"matrixProviderMs\": " << g_timing.matrixProviderMs
        << L", \"viewportProviderMs\": " << g_timing.viewportProviderMs
        << L", \"projected\": " << g_timing.projectedCount
        << L", \"clipped\": " << g_timing.clippedCount << L" },\n";
    out << L"  \"entities\": [\n";
    for (std::size_t index = 0; index < g_entities.size(); ++index)
    {
        const AegisIdTechEntitySnapshot& entity = g_entities[index];
        out << L"    { \"id\": " << entity.id
            << L", \"name\": \"" << JsonEscape(entity.name).c_str()
            << L"\", \"className\": \"" << JsonEscape(entity.className).c_str()
            << L"\", \"origin\": [" << entity.origin.x << L", " << entity.origin.y << L", " << entity.origin.z
            << L"], \"boundsMin\": [" << entity.boundsMin.x << L", " << entity.boundsMin.y << L", " << entity.boundsMin.z
            << L"], \"boundsMax\": [" << entity.boundsMax.x << L", " << entity.boundsMax.y << L", " << entity.boundsMax.z
            << L"], \"team\": " << entity.team
            << L", \"visible\": " << entity.visible
            << L", \"flags\": " << entity.flags << L" }";
        if (index + 1 < g_entities.size())
            out << L",";
        out << L"\n";
    }
    out << L"  ]\n";
    out << L"}\n";
    return 1;
}

AEGIS_UNIVERSAL_API int AegisIdTech_LoadSnapshotJson(const wchar_t* path)
{
    if (!path || !path[0])
        return 0;

    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in)
        return 0;

    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    AegisIdTechViewport viewport = {};
    ExtractJsonNumber(json, "x", viewport.x);
    ExtractJsonNumber(json, "y", viewport.y);
    ExtractJsonNumber(json, "width", viewport.width);
    ExtractJsonNumber(json, "height", viewport.height);

    AegisIdTechMatrix4x4 matrix = {};
    const bool hasMatrix = ExtractJsonMatrix(json, matrix);

    std::vector<AegisIdTechEntitySnapshot> entities;
    std::size_t arrayPos = json.find("\"entities\"");
    if (arrayPos != std::string::npos)
    {
        std::size_t pos = json.find('{', arrayPos);
        while (pos != std::string::npos)
        {
            const std::size_t end = json.find('}', pos);
            if (end == std::string::npos)
                break;
            const std::string object = json.substr(pos, end - pos + 1);

            AegisIdTechEntitySnapshot entity = {};
            ExtractJsonUInt64(object, "id", entity.id);
            CopyAnsi(entity.name, ExtractJsonString(object, "name"));
            CopyAnsi(entity.className, ExtractJsonString(object, "className"));
            ExtractJsonVec3(object, "origin", entity.origin);
            ExtractJsonVec3(object, "boundsMin", entity.boundsMin);
            ExtractJsonVec3(object, "boundsMax", entity.boundsMax);
            ExtractJsonInt(object, "team", entity.team);
            ExtractJsonInt(object, "visible", entity.visible);
            std::int32_t flags = 0;
            ExtractJsonInt(object, "flags", flags);
            entity.flags = static_cast<std::uint32_t>(flags);
            entities.push_back(entity);

            pos = json.find('{', end + 1);
        }
    }

    std::lock_guard lock(g_adapterMutex);
    if (IsValidViewport(viewport))
    {
        g_viewport = viewport;
        g_hasViewport = true;
    }
    if (hasMatrix && IsValidMatrix(matrix))
    {
        g_viewProjection = matrix;
        g_hasMatrix = true;
    }
    g_entities = std::move(entities);
    ++g_timing.frameId;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API void AegisIdTech_PrintCurrentEntities()
{
    std::vector<AegisIdTechEntitySnapshot> snapshot;
    {
        std::lock_guard lock(g_adapterMutex);
        snapshot = g_entities;
    }

    HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
    for (const AegisIdTechEntitySnapshot& entity : snapshot)
    {
        std::ostringstream line;
        line << "[IdTechUniversal] Entity id=" << entity.id
             << " name=\"" << entity.name
             << "\" class=\"" << entity.className
             << "\" origin=(" << entity.origin.x << ", " << entity.origin.y << ", " << entity.origin.z << ")\n";
        const std::string text = line.str();
        ::OutputDebugStringA(text.c_str());
        if (output && output != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            if (!::WriteConsoleA(output, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr))
                ::WriteFile(output, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
        }
    }
}
