#pragma once

#include <memory>

class FogInfo
{
  public:
    struct FinalizedInfo
    {
        float linearFog_nearDist = 0.0f, linearFog_farDist = 0.0f, expFog_density = 0.0f,
              marchedFog_defaultDensity = 0.0f, marchedFog_sigmaAbsorption = 0.0f, marchedFog_lightPropertyDirG = 0.0f;
        int marchedFog_numMainSteps = 0;

        FinalizedInfo(const float &linearFog_nearDist, const float &linearFog_farDist, const float &expFog_density,
                      const float &marchedFog_defaultDensity, const float &marchedFog_sigmaAbsorption,
                      const float &marchedFog_lightPropertyDirG, const int &marchedFog_numMainSteps)
            : linearFog_nearDist(linearFog_nearDist), linearFog_farDist(linearFog_farDist),
              expFog_density(expFog_density), marchedFog_defaultDensity(marchedFog_defaultDensity),
              marchedFog_sigmaAbsorption(marchedFog_sigmaAbsorption),
              marchedFog_lightPropertyDirG(marchedFog_lightPropertyDirG),
              marchedFog_numMainSteps(marchedFog_numMainSteps)
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

        LinearFogInfo &operator=(const LinearFogInfo &other)
        {
            this->nearDist = other.nearDist;
            this->farDist = other.farDist;

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

        ExpFogInfo &operator=(const ExpFogInfo &other)
        {
            this->density = other.density;

            return *this;
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
        float defaultDensity = 0.0f, sigmaAbsorption = 0.0f, lightPropertyDirG = 0.0f;
        int numMainSteps = 0;

        MarchedFogInfo() = default;

        MarchedFogInfo(const float &defaultDensity, const float &sigmaAbsorption, const float &lightPropertyDirG,
                       const int &numMainSteps)
            : defaultDensity(defaultDensity), sigmaAbsorption(sigmaAbsorption), lightPropertyDirG(lightPropertyDirG),
              numMainSteps(numMainSteps)
        {
        }

        MarchedFogInfo(const MarchedFogInfo &other)
            : defaultDensity(other.defaultDensity), sigmaAbsorption(other.sigmaAbsorption),
              lightPropertyDirG(other.lightPropertyDirG), numMainSteps(other.numMainSteps)
        {
        }

        MarchedFogInfo &operator=(const MarchedFogInfo &other)
        {
            this->defaultDensity = other.defaultDensity;
            this->sigmaAbsorption = other.sigmaAbsorption;
            this->lightPropertyDirG = other.lightPropertyDirG;
            this->numMainSteps = other.numMainSteps;

            return *this;
        }

        bool operator==(const MarchedFogInfo &other) const
        {
            return this->defaultDensity == other.defaultDensity && this->sigmaAbsorption == other.sigmaAbsorption &&
                   this->lightPropertyDirG == other.lightPropertyDirG && this->numMainSteps == other.numMainSteps;
        }

        bool operator!=(const MarchedFogInfo &other) const
        {
            return this->defaultDensity != other.defaultDensity || this->sigmaAbsorption != other.sigmaAbsorption ||
                   this->lightPropertyDirG != other.lightPropertyDirG || this->numMainSteps != other.numMainSteps;
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
        return FinalizedInfo{this->linearInfo.nearDist,         this->linearInfo.farDist,
                             this->expFogInfo.density,          this->marchedInfo.defaultDensity,
                             this->marchedInfo.sigmaAbsorption, this->marchedInfo.lightPropertyDirG,
                             this->marchedInfo.numMainSteps};
    }

  private:
};