#include "controller/detail/SimulationBoundsFile.hpp"

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace controller
{
static void to_json(json &j, const FogInfo::LinearFogInfo &v)
{
    j = json{{"nearDist", v.nearDist}, {"farDist", v.farDist}};
}

static void from_json(const json &j, FogInfo::LinearFogInfo &v)
{
    // tolerant to missing keys; falls back to defaults already in v
    v.nearDist = j.value("nearDist", v.nearDist);
    v.farDist = j.value("farDist", v.farDist);
}

static void to_json(json &j, const FogInfo::ExpFogInfo &v)
{
    j = json{{"density", v.density}};
}

static void from_json(const json &j, FogInfo::ExpFogInfo &v)
{
    v.density = j.value("density", v.density);
}

static void to_json(json &j, const FogInfo::MarchedFogInfo &v)
{
    j = json{{"defaultDensity", v.defaultDensity},
             {"sigmaAbsorption", v.getSigmaAbsorption()},
             {"sigmaScattering", v.getSigmaScattering()},
             {"lightPropertyDirG", v.getLightPropertyDirG()},
             {"stepSizeDist", v.stepSizeDist},
             {"stepSizeDist_light", v.stepSizeDist_light}};
}

int SimulationBoundsFile::operator()(const std::string &filePath)
{
    SimulationBounds startBounds; 
    SimulationBounds endBounds; 

    m_loadedBounds.set_value({}); 
    return 0;
}

SimulationBoundsFile::SimulationBoundsFile(std::string filePath)
    : m_filePath(std::move(filePath))
{
}
} // namespace controller
