#include "FogInfo_json.hpp"
#include "structs/ExpFogInfo.hpp"
#include "structs/FogInfo.hpp"
#include "structs/HomogenousRendering.hpp"
#include "structs/LinearFogInfo.hpp"
#include "structs/MarchedFogInfo.hpp"

void to_json(nlohmann::json &j, const LinearFogInfo &v)
{
    j = nlohmann::json{{"nearDist", v.nearDist}, {"farDist", v.farDist}};
}

void from_json(const nlohmann::json &j, LinearFogInfo &v)
{
    v.nearDist = j.value("nearDist", v.nearDist);
    v.farDist = j.value("farDist", v.farDist);
}

void to_json(nlohmann::json &j, const ExpFogInfo &v)
{
    j = nlohmann::json{{"density", v.density}};
}

void from_json(const nlohmann::json &j, ExpFogInfo &v)
{
    v.density = j.value("density", v.density);
}

void to_json(nlohmann::json &j, const MarchedFogInfo &v)
{
    j = nlohmann::json{{"defaultDensity", v.defaultDensity},
                       {"sigmaAbsorption", v.getSigmaAbsorption()},
                       {"sigmaScattering", v.getSigmaScattering()},
                       {"lightPropertyDirG", v.getLightPropertyDirG()},
                       {"stepSizeDist", v.stepSizeDist},
                       {"stepSizeDist_light", v.stepSizeDist_light},
                       {"densityMultiplier", v.getDensityMultiplier()},
                       {"colorTransparencyCutoff", v.getColorTransparencyCutoff()},
                       {"distanceTransparencyCutoff", v.getDistanceTransparencyCutoff()},
                       {"lightExtinctionScale", v.getLightExtinctionScale()}};
}

void from_json(const nlohmann::json &j, MarchedFogInfo &v)
{
    // Public fields
    v.defaultDensity = j.value("defaultDensity", v.defaultDensity);
    v.stepSizeDist = j.value("stepSizeDist", v.stepSizeDist);
    v.stepSizeDist_light = j.value("stepSizeDist_light", v.stepSizeDist_light);

    // Private fields via setters
    const float sigmaAbs = j.value("sigmaAbsorption", v.getSigmaAbsorption());
    const float sigmaSca = j.value("sigmaScattering", v.getSigmaScattering());
    const float g = j.value("lightPropertyDirG", v.getLightPropertyDirG());
    const float densityMulti = j.value("densityMultiplier", v.getDensityMultiplier());
    const float colorTransparencyCutoff = j.value(
        "colorTransparencyCutoff", j.value("cutoffValue", v.getColorTransparencyCutoff()));
    const float distanceTransparencyCutoff = j.value(
        "distanceTransparencyCutoff", colorTransparencyCutoff);
    const float lightExtinctionScale = j.value("lightExtinctionScale", v.getLightExtinctionScale());

    v.setSigmaAbsorption(sigmaAbs);
    v.setSigmaScattering(sigmaSca);
    v.setLightPropertyDirG(g);
    v.setDensityMultiplier(densityMulti);
    v.setColorTransparencyCutoff(colorTransparencyCutoff);
    v.setDistanceTransparencyCutoff(distanceTransparencyCutoff);
    v.setLightExtinctionScale(lightExtinctionScale);
}

void to_json(nlohmann::json &j, const HomogenousRendering &v)
{
    j = nlohmann::json{{"maxNumSteps", v.maxNumSteps}};
}

void from_json(const nlohmann::json &j, HomogenousRendering &v)
{
    v.maxNumSteps = j.value("maxNumSteps", v.maxNumSteps);
}

void to_json(nlohmann::json &j, const FogInfo &v)
{
    nlohmann::json linearData;
    to_json(linearData, v.linearInfo);
    nlohmann::json expData;
    to_json(expData, v.expFogInfo);
    nlohmann::json marchedData;
    to_json(marchedData, v.marchedInfo);
    nlohmann::json homoData;
    to_json(homoData, v.homogenousInfo);

    j = nlohmann::json{{"linearInfo", linearData},
                       {"expFogInfo", expData},
                       {"marchedInfo", marchedData},
                       {"homogenousInfo", homoData}};
}

void from_json(const nlohmann::json &j, FogInfo &v)
{
    // Use existing defaults if sections are absent
    from_json(j["linearInfo"], v.linearInfo);
    from_json(j["expFogInfo"], v.expFogInfo);
    from_json(j["marchedInfo"], v.marchedInfo);
    from_json(j["homogenousInfo"], v.homogenousInfo);
}