#include "util/json/FogInfoStruct.hpp"

void util::to_json(nlohmann::json &j, const ::FogInfo::LinearFogInfo &v)
{
    j = nlohmann::json{{"nearDist", v.nearDist}, {"farDist", v.farDist}};
}

void util::from_json(const nlohmann::json &j, ::FogInfo::LinearFogInfo &v)
{
    // Preserve defaults if keys are absent
    v.nearDist = j.value("nearDist", v.nearDist);
    v.farDist = j.value("farDist", v.farDist);
}

// -------------------- ExpFogInfo --------------------
void util::to_json(nlohmann::json & j, const FogInfo::ExpFogInfo &v)
{
    j = nlohmann::json{{"density", v.density}};
}

void util::from_json(const nlohmann::json &j, FogInfo::ExpFogInfo &v)
{
    v.density = j.value("density", v.density);
}

// -------------------- MarchedFogInfo --------------------
void util::to_json(nlohmann::json &j, const FogInfo::MarchedFogInfo &v)
{
    j = nlohmann::json{{"defaultDensity", v.defaultDensity},
             {"sigmaAbsorption", v.getSigmaAbsorption()},
             {"sigmaScattering", v.getSigmaScattering()},
             {"lightPropertyDirG", v.getLightPropertyDirG()},
             {"stepSizeDist", v.stepSizeDist},
             {"stepSizeDist_light", v.stepSizeDist_light}};
}

void util::from_json(const nlohmann::json &j, FogInfo::MarchedFogInfo &v)
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
void util::to_json(nlohmann::json &j, const FogInfo::HomogenousRendering &v)
{
    j = nlohmann::json{{"maxNumSteps", v.maxNumSteps}};
}

void util::from_json(const nlohmann::json &j, FogInfo::HomogenousRendering &v)
{
    v.maxNumSteps = j.value("maxNumSteps", v.maxNumSteps);
}

// -------------------- FogInfo (root) --------------------
void util::to_json(nlohmann::json &j, const FogInfo &v)
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

void util::from_json(const nlohmann::json &j, FogInfo &v)
{
    // Use existing defaults if sections are absent
    from_json(j["linearInfo"], v.linearInfo);
    from_json(j["expFogInfo"], v.expFogInfo);
    from_json(j["marchedInfo"], v.marchedInfo);
    from_json(j["homogenousInfo"], v.homogenousInfo);
}