#pragma once

#include "structs/ExpFogInfo.hpp"
#include "structs/LinearFogInfo.hpp"
#include "structs/MarchedFogInfo.hpp"
#include "structs/HomogenousRendering.hpp"
#include <memory>

class FogInfo
{
  public:
    struct FinalizedInfo
    {
        float linearFog_nearDist = 0.0f, linearFog_farDist = 0.0f, expFog_density = 0.0f,
              marchedFog_defaultDensity = 0.0f, marchedFog_sigmaAbsorption = 0.0f, marchedFog_sigmaScattering = 0.0f,
              marchedFog_lightPropertyDirG = 0.0f, marchedFog_stepSizeDist = 1.0f, marchedFog_stepSizeDist_light = 3.0f,
              marchedFog_densityMultiplier = 0.0f, marchedFog_colorTransparencyCutoff = 0.01f,
              marchedFog_distanceTransparencyCutoff = 0.01f, marchedFog_lightExtinctionScale = 1.0f;
        uint32_t marchedFog_maxNumSteps = 25600, vdb_gridType = 0;
    };

    LinearFogInfo linearInfo;
    ExpFogInfo expFogInfo;
    MarchedFogInfo marchedInfo;
    HomogenousRendering homogenousInfo;

    FogInfo() = default;

    FogInfo(const LinearFogInfo &linearInfo, const ExpFogInfo &expFogInfo, const MarchedFogInfo &marchedInfo,
            const HomogenousRendering &homoInfo)
        : linearInfo(linearInfo), expFogInfo(expFogInfo), marchedInfo(marchedInfo), homogenousInfo(homoInfo)
    {
    }

    FogInfo(const FogInfo &other)
        : linearInfo(other.linearInfo), expFogInfo(other.expFogInfo), marchedInfo(other.marchedInfo),
          homogenousInfo(other.homogenousInfo)
    {
    }

    FogInfo &operator=(const FogInfo &other)
    {
        this->linearInfo = other.linearInfo;
        this->expFogInfo = other.expFogInfo;
        this->marchedInfo = other.marchedInfo;
        this->homogenousInfo = other.homogenousInfo;

        return *this;
    }

    bool operator==(const FogInfo &other) const
    {
        return this->linearInfo == other.linearInfo && this->expFogInfo == other.expFogInfo &&
               this->marchedInfo == other.marchedInfo && this->homogenousInfo == other.homogenousInfo;
    }

    bool operator!=(const FogInfo &other) const
    {
        return this->linearInfo != other.linearInfo || this->expFogInfo != other.expFogInfo ||
               this->marchedInfo != other.marchedInfo || this->homogenousInfo != other.homogenousInfo;
    }

    FinalizedInfo getInfo() const
    {
        return FinalizedInfo{this->linearInfo.nearDist,
                             this->linearInfo.farDist,
                             this->expFogInfo.density,
                             this->marchedInfo.defaultDensity,
                             this->marchedInfo.getSigmaAbsorption(),
                             this->marchedInfo.getSigmaScattering(),
                             this->marchedInfo.getLightPropertyDirG(),
                             this->marchedInfo.stepSizeDist,
                             this->marchedInfo.stepSizeDist_light,
                             this->marchedInfo.getDensityMultiplier(),
                             this->marchedInfo.getColorTransparencyCutoff(),
                             this->marchedInfo.getDistanceTransparencyCutoff(),
                             this->marchedInfo.getLightExtinctionScale(),
                             this->homogenousInfo.maxNumSteps,
                             uint32_t(0)};
    }
};