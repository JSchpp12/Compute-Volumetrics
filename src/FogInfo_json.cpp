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
    v.nearDist = j.at("nearDist").get<float>();
    v.farDist = j.at("farDist").get<float>();
}

void to_json(nlohmann::json &j, const ExpFogInfo &v)
{
    j = nlohmann::json{{"density", v.density}};
}

void from_json(const nlohmann::json &j, ExpFogInfo &v)
{
    v.density = j.at("density").get<float>();
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
    v.defaultDensity = j.at("defaultDensity").get<float>();
    v.stepSizeDist = j.at("stepSizeDist").get<float>();
    v.stepSizeDist_light = j.at("stepSizeDist_light").get<float>();

    // Private fields via setters
    const float sigmaAbs = j.at("sigmaAbsorption").get<float>();
    const float sigmaSca = j.at("sigmaScattering").get<float>();
    const float g = j.at("lightPropertyDirG").get<float>();
    const float densityMulti = j.at("densityMultiplier").get<float>();
    const float colorTransparencyCutoff = j.at("colorTransparencyCutoff").get<float>();
    const float distanceTransparencyCutoff = j.at("distanceTransparencyCutoff").get<float>();
    const float lightExtinctionScale = j.at("lightExtinctionScale").get<float>();

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
    if (j.contains("linearInfo"))
        from_json(j["linearInfo"], v.linearInfo);
    if (j.contains("expFogInfo"))
        from_json(j["expFogInfo"], v.expFogInfo);
    if (j.contains("marchedInfo"))
        from_json(j["marchedInfo"], v.marchedInfo);
    if (j.contains("homogenousInfo"))
        from_json(j["homogenousInfo"], v.homogenousInfo);
}