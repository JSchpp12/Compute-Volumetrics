#pragma once

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
