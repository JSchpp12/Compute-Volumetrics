#include "fog_info/JsonUtils.hpp"

#include <fstream>
#include <string>

namespace fog_info
{
void to_json(json &j, const FogInfo::LinearFogInfo &v)
{
    j = json{{"nearDist", v.nearDist}, {"farDist", v.farDist}};
}

void from_json(const json &j, FogInfo::LinearFogInfo &v)
{
    // Preserve defaults if keys are absent
    v.nearDist = j.value("nearDist", v.nearDist);
    v.farDist = j.value("farDist", v.farDist);
}

// -------------------- ExpFogInfo --------------------
void to_json(json &j, const FogInfo::ExpFogInfo &v)
{
    j = json{{"density", v.density}};
}

void from_json(const json &j, FogInfo::ExpFogInfo &v)
{
    v.density = j.value("density", v.density);
}

// -------------------- MarchedFogInfo --------------------
void to_json(json &j, const FogInfo::MarchedFogInfo &v)
{
    j = json{{"defaultDensity", v.defaultDensity},
             {"sigmaAbsorption", v.getSigmaAbsorption()},
             {"sigmaScattering", v.getSigmaScattering()},
             {"lightPropertyDirG", v.getLightPropertyDirG()},
             {"stepSizeDist", v.stepSizeDist},
             {"stepSizeDist_light", v.stepSizeDist_light}};
}

void from_json(const json &j, FogInfo::MarchedFogInfo &v)
{
    // Public fields
    v.defaultDensity = j.value("defaultDensity", v.defaultDensity);
    v.stepSizeDist = j.value("stepSizeDist", v.stepSizeDist);
    v.stepSizeDist_light = j.value("stepSizeDist_light", v.stepSizeDist_light);

    // Private fields via setters
    const float sigmaAbs = j.value("sigmaAbsorption", v.getSigmaAbsorption());
    const float sigmaSca = j.value("sigmaScattering", v.getSigmaScattering());
    const float g = j.value("lightPropertyDirG", v.getLightPropertyDirG());

    v.setSigmaAbsorption(sigmaAbs);
    v.setSigmaScattering(sigmaSca);
    v.setLightPropertyDirG(g);
}

// -------------------- HomogenousRendering --------------------
void to_json(json &j, const FogInfo::HomogenousRendering &v)
{
    j = json{{"maxNumSteps", v.maxNumSteps}};
}

void from_json(const json &j, FogInfo::HomogenousRendering &v)
{
    v.maxNumSteps = j.value("maxNumSteps", v.maxNumSteps);
}

// -------------------- FogInfo (root) --------------------
void to_json(json &j, const FogInfo &v)
{
    json linearData;
    to_json(linearData, v.linearInfo);
    json expData;
    to_json(expData, v.expFogInfo);
    json marchedData;
    to_json(marchedData, v.marchedInfo);
    json homoData;
    to_json(homoData, v.homogenousInfo);

    j = json{{"linearInfo", linearData},
             {"expFogInfo", expData},
             {"marchedInfo", marchedData},
             {"homogenousInfo", homoData}};
}

void from_json(const json &j, FogInfo &v)
{
    // Use existing defaults if sections are absent
    from_json(j["linearInfo"], v.linearInfo);
    from_json(j["expFogInfo"], v.expFogInfo);
    from_json(j["marchedInfo"], v.marchedInfo);
    from_json(j["homogenousInfo"], v.homogenousInfo);
}

// -------------------- File helpers --------------------
bool saveFogInfoToFile(const FogInfo &fog, const std::filesystem::path &filePath, int indentSpaces = 2,
                       std::string *errorOut = nullptr)
{
    try
    {
        json j;
        to_json(j, fog);

        std::ofstream os(filePath, std::ios::binary | std::ios::trunc);
        if (!os)
        {
            if (errorOut)
                *errorOut = "Failed to open file for writing: " + filePath.string();
            return false;
        }
        os << j.dump(indentSpaces);
        return true;
    }
    catch (const std::exception &e)
    {
        if (errorOut)
            *errorOut = std::string("Exception while saving JSON: ") + e.what();
        return false;
    }
}

std::optional<FogInfo> loadFogInfoFromFile(const std::filesystem::path &filePath, std::string *errorOut = nullptr)
{
    try
    {
        std::ifstream is(filePath, std::ios::binary);
        if (!is)
        {
            if (errorOut)
                *errorOut = "Failed to open file for reading: " + filePath.string();
            return std::nullopt;
        }
        json j;
        is >> j;

        FogInfo fog;
        from_json(j, fog);
        return fog;
    }
    catch (const std::exception &e)
    {
        if (errorOut)
            *errorOut = std::string("Exception while loading JSON: ") + e.what();
        return std::nullopt;
    }
}
} // namespace fog_info