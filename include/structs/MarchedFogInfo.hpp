#pragma once

struct MarchedFogInfo
{
    float defaultDensity = 0.0f, stepSizeDist = 0.0f, stepSizeDist_light = 0.0f;

    MarchedFogInfo() = default;

    MarchedFogInfo(const float &defaultDensity, const float &sigmaAbsorption, const float &sigmaScattering,
                   const float &lightPropertyDirG, const float &stepSizeDist, const float &stepSizeDist_light,
                float densityMultiplier, float colorTransparencyCutoff,
                    float distanceTransparencyCutoff, float lightExtinctionScale)
        : defaultDensity(defaultDensity), stepSizeDist(stepSizeDist), stepSizeDist_light(stepSizeDist_light),
          lightPropertyDirG(lightPropertyDirG), sigmaAbsorption(sigmaAbsorption), sigmaScattering(sigmaScattering),
          densityMultiplier(densityMultiplier), colorTransparencyCutoff(colorTransparencyCutoff),
          distanceTransparencyCutoff(distanceTransparencyCutoff), lightExtinctionScale(lightExtinctionScale)
    {
    }

    MarchedFogInfo(const MarchedFogInfo &other)
        : defaultDensity(other.defaultDensity), stepSizeDist(other.stepSizeDist),
          stepSizeDist_light(other.stepSizeDist_light), lightPropertyDirG(other.lightPropertyDirG),
          sigmaAbsorption(other.sigmaAbsorption), sigmaScattering(other.sigmaScattering),
          densityMultiplier(other.densityMultiplier), colorTransparencyCutoff(other.colorTransparencyCutoff),
          distanceTransparencyCutoff(other.distanceTransparencyCutoff),
          lightExtinctionScale(other.lightExtinctionScale)
    {
    }

    MarchedFogInfo &operator=(const MarchedFogInfo &other)
    {
        if (this != &other)
        {
            this->defaultDensity = other.defaultDensity;
            this->sigmaAbsorption = other.sigmaAbsorption;
            this->sigmaScattering = other.sigmaScattering;
            this->lightPropertyDirG = other.lightPropertyDirG;
            this->stepSizeDist = other.stepSizeDist;
            this->stepSizeDist_light = other.stepSizeDist_light;
            this->densityMultiplier = other.densityMultiplier;
            this->colorTransparencyCutoff = other.colorTransparencyCutoff;
            this->distanceTransparencyCutoff = other.distanceTransparencyCutoff;
            this->lightExtinctionScale = other.lightExtinctionScale;
        }

        return *this;
    }

    bool operator==(const MarchedFogInfo &other) const
    {
        return this->defaultDensity == other.defaultDensity && this->sigmaAbsorption == other.sigmaAbsorption &&
               this->sigmaScattering == other.sigmaScattering && this->lightPropertyDirG == other.lightPropertyDirG &&
               this->stepSizeDist == other.stepSizeDist && this->stepSizeDist_light == other.stepSizeDist_light &&
               this->densityMultiplier == other.densityMultiplier &&
               this->colorTransparencyCutoff == other.colorTransparencyCutoff &&
               this->distanceTransparencyCutoff == other.distanceTransparencyCutoff &&
               this->lightExtinctionScale == other.lightExtinctionScale;
    }

    bool operator!=(const MarchedFogInfo &other) const
    {
        return this->defaultDensity != other.defaultDensity || this->sigmaAbsorption != other.sigmaAbsorption ||
               this->sigmaScattering != other.sigmaScattering || this->lightPropertyDirG != other.lightPropertyDirG ||
               this->stepSizeDist != other.stepSizeDist || this->stepSizeDist_light != other.stepSizeDist_light ||
               this->densityMultiplier != other.densityMultiplier ||
               this->colorTransparencyCutoff != other.colorTransparencyCutoff ||
               this->distanceTransparencyCutoff != other.distanceTransparencyCutoff ||
               this->lightExtinctionScale != other.lightExtinctionScale;
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
        else
        {
            lightPropertyDirG = value;
        }
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

    void setDensityMultiplier(const float &value)
    {
        densityMultiplier = value;
    }

    float getDensityMultiplier() const
    {
        return densityMultiplier;
    }

    void setColorTransparencyCutoff(float value)
    {
        colorTransparencyCutoff = std::move(value);
    }

    float getColorTransparencyCutoff() const
    {
        return colorTransparencyCutoff;
    }

    void setDistanceTransparencyCutoff(float value)
    {
        distanceTransparencyCutoff = std::move(value);
    }

    float getDistanceTransparencyCutoff() const
    {
        return distanceTransparencyCutoff;
    }

    void setLightExtinctionScale(float value)
    {
        lightExtinctionScale = std::move(value);
    }

    float getLightExtinctionScale() const
    {
        return lightExtinctionScale;
    }

  private:
    float lightPropertyDirG = 0.0f, sigmaAbsorption = 0.0f, sigmaScattering = 0.0f, densityMultiplier = 1.0f,
          colorTransparencyCutoff = 0.01f, distanceTransparencyCutoff = 0.01f, lightExtinctionScale = 1.0f;

    bool validateSigmaTotal(const float &sigmaAbsorption, const float &sigmaScattering) const
    {
        const float sigmaTotal = sigmaAbsorption + sigmaScattering;
        return sigmaTotal <= 1.0f && sigmaTotal >= 0.0f;
    }
};