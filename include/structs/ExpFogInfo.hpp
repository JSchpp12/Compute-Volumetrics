#pragma once

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