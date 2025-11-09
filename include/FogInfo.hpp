#pragma once

#include <memory>

class FogInfo
{
  public:
    struct FinalizedInfo
    {
        float linearFog_nearDist = 0.0f, linearFog_farDist = 0.0f, expFog_density = 0.0f,
              marchedFog_defaultDensity = 0.0f, marchedFog_sigmaAbsorption = 0.0f, marchedFog_sigmaScattering = 0.0f,
              marchedFog_lightPropertyDirG = 0.0f, marchedFog_stepSizeDist = 1.0f, marchedFog_stepSizeDist_light = 3.0f;
        uint32_t marchedFog_levelSet_maxNumSteps = 1000, vdb_gridType = 0;

        FinalizedInfo(const float &linearFog_nearDist, const float &linearFog_farDist, const float &expFog_density,
                      const float &marchedFog_defaultDensity, const float &marchedFog_sigmaAbsorption,
                      const float &marchedFog_sigmaScattering, const float &marchedFog_lightPropertyDirG,
                      const float &marchedFog_stepSizeDist, const float &marchedFog_stepSizeDist_light,
                      uint32_t marchedFog_levelSet_maxNumSteps, uint32_t vdb_gridType)
            : linearFog_nearDist(linearFog_nearDist), linearFog_farDist(linearFog_farDist),
              expFog_density(expFog_density), marchedFog_defaultDensity(marchedFog_defaultDensity),
              marchedFog_sigmaAbsorption(marchedFog_sigmaAbsorption),
              marchedFog_sigmaScattering(marchedFog_sigmaScattering),
              marchedFog_lightPropertyDirG(marchedFog_lightPropertyDirG),
              marchedFog_stepSizeDist(marchedFog_stepSizeDist),
              marchedFog_stepSizeDist_light(marchedFog_stepSizeDist_light),
              marchedFog_levelSet_maxNumSteps(std::move(marchedFog_levelSet_maxNumSteps)),
              vdb_gridType(std::move(vdb_gridType))
        {
        }
    };

    struct LinearFogInfo
    {
        float nearDist = 0.0f, farDist = 0.0f;

        LinearFogInfo() = default;

        LinearFogInfo(const float &nearDist, const float &farDist) : nearDist(nearDist), farDist(farDist)
        {
        }

        LinearFogInfo(const LinearFogInfo &other) : nearDist(other.nearDist), farDist(other.farDist)
        {
        }
        LinearFogInfo(LinearFogInfo &&other) : nearDist(std::move(other.nearDist)), farDist(std::move(other.farDist))
        {
        }
        LinearFogInfo &operator=(const LinearFogInfo &other)
        {
            if (this != &other)
            {
                this->nearDist = other.nearDist;
                this->farDist = other.farDist;
            }

            return *this;
        }
        LinearFogInfo &operator=(LinearFogInfo &&other)
        {
            if (this != &other)
            {
                nearDist = std::move(other.nearDist);
                farDist = std::move(other.farDist);
            }

            return *this;
        }

        bool operator==(const LinearFogInfo &other) const
        {
            return this->nearDist == other.nearDist && this->farDist == other.farDist;
        }

        bool operator!=(const LinearFogInfo &other) const
        {
            return this->nearDist != other.nearDist || this->farDist != other.farDist;
        }
    };

    struct ExpFogInfo
    {
        float density = 0.0f;

        ExpFogInfo() = default;

        ExpFogInfo(const float &density) : density(density)
        {
        }

        bool operator==(const ExpFogInfo &other) const
        {
            return this->density == other.density;
        }

        bool operator!=(const ExpFogInfo &other) const
        {
            return this->density != other.density;
        }
    };

    struct MarchedFogInfo
    {
        float defaultDensity = 0.0f, stepSizeDist = 0.0f, stepSizeDist_light = 0.0f;

        MarchedFogInfo() = default;

        MarchedFogInfo(const float &defaultDensity, const float &sigmaAbsorption, const float &sigmaScattering,
                       const float &lightPropertyDirG, const float &stepSizeDist, const float &stepSizeDist_light)
            : defaultDensity(defaultDensity), sigmaAbsorption(sigmaAbsorption), sigmaScattering(sigmaScattering),
              lightPropertyDirG(lightPropertyDirG), stepSizeDist(stepSizeDist), stepSizeDist_light(stepSizeDist_light)
        {
        }

        MarchedFogInfo(const MarchedFogInfo &other)
            : defaultDensity(other.defaultDensity), sigmaAbsorption(other.sigmaAbsorption),
              lightPropertyDirG(other.lightPropertyDirG), stepSizeDist(other.stepSizeDist),
              stepSizeDist_light(other.stepSizeDist_light)
        {
        }

        MarchedFogInfo &operator=(const MarchedFogInfo &other)
        {
            this->defaultDensity = other.defaultDensity;
            this->sigmaAbsorption = other.sigmaAbsorption;
            this->lightPropertyDirG = other.lightPropertyDirG;
            this->stepSizeDist = other.stepSizeDist;
            this->stepSizeDist_light = other.stepSizeDist_light;

            return *this;
        }

        bool operator==(const MarchedFogInfo &other) const
        {
            return this->defaultDensity == other.defaultDensity && this->sigmaAbsorption == other.sigmaAbsorption &&
                   this->lightPropertyDirG == other.lightPropertyDirG && this->stepSizeDist == other.stepSizeDist &&
                   this->stepSizeDist_light == other.stepSizeDist_light;
        }

        bool operator!=(const MarchedFogInfo &other) const
        {
            return this->defaultDensity != other.defaultDensity || this->sigmaAbsorption != other.sigmaAbsorption ||
                   this->lightPropertyDirG != other.lightPropertyDirG || this->stepSizeDist != other.stepSizeDist ||
                   this->stepSizeDist_light != other.stepSizeDist_light;
        }

        float getLightPropertyDirG() const
        {
            return lightPropertyDirG;
        }

        void setLightPropertyDirG(const float &value)
        {
            if (value < -1.0f || value > 1.0f)
            {
                lightPropertyDirG = 0.0;
            }

            lightPropertyDirG = value;
        }

        void setSigmaAbsorption(const float &value)
        {
            sigmaAbsorption = value;
        }

        float getSigmaAbsorption() const
        {
            return sigmaAbsorption;
        }

        void setSigmaScattering(const float &value)
        {
            sigmaScattering = value; 
        }

        float getSigmaScattering() const
        {
            return sigmaScattering;
        }

      private:
        float lightPropertyDirG = 0.0f, sigmaAbsorption = 0.0f, sigmaScattering = 0.0f;

        bool validateSigmaTotal(const float &sigmaAbsorption, const float &sigmaScattering) const
        {
            const float sigmaTotal = sigmaAbsorption + sigmaScattering;
            return sigmaTotal <= 1.0f && sigmaTotal >= 0.0f;
        }
    };

    LinearFogInfo linearInfo = LinearFogInfo();
    ExpFogInfo expFogInfo = ExpFogInfo();
    MarchedFogInfo marchedInfo = MarchedFogInfo();

    FogInfo() = default;

    FogInfo(const LinearFogInfo &linearInfo, const ExpFogInfo &expFogInfo, const MarchedFogInfo &marchedInfo)
        : linearInfo(linearInfo), expFogInfo(expFogInfo), marchedInfo(marchedInfo)
    {
    }

    FogInfo(const FogInfo &other)
        : linearInfo(other.linearInfo), expFogInfo(other.expFogInfo), marchedInfo(other.marchedInfo)
    {
    }

    FogInfo &operator=(const FogInfo &other)
    {
        this->linearInfo = other.linearInfo;
        this->expFogInfo = other.expFogInfo;
        this->marchedInfo = other.marchedInfo;

        return *this;
    }

    bool operator==(const FogInfo &other) const
    {
        return this->linearInfo == other.linearInfo && this->expFogInfo == other.expFogInfo &&
               this->marchedInfo == other.marchedInfo;
    }

    bool operator!=(const FogInfo &other) const
    {
        return this->linearInfo != other.linearInfo || this->expFogInfo != other.expFogInfo ||
               this->marchedInfo != other.marchedInfo;
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
                             uint32_t(1000),
                             uint32_t(10)};
    }
};