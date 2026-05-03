#pragma once

#include <string>

enum TerrainRenderingType
{
	Flat, 
	Real
};

constexpr std::string toString(TerrainRenderingType type)
{
    switch (type)
    {
    case (TerrainRenderingType::Flat):
        return "flat";
    case (TerrainRenderingType::Real):
        return "real";
    }

    return "";
}