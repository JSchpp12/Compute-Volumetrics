#pragma once

#include <nlohmann/json.hpp>

#include "FogInfo.hpp"

using nlohmann::json;

namespace fog_info
{
void to_json(json &j, const FogInfo::LinearFogInfo &v);
void from_json(const json &j, FogInfo::LinearFogInfo &v); 
void to_json(json &j, const FogInfo::ExpFogInfo &v); 
void from_json(const json& j, FogInfo::ExpFogInfo& v);
void to_json(json& j, const FogInfo::MarchedFogInfo& v);
void from_json(const json& j, FogInfo::MarchedFogInfo& v); 
void to_json(json& j, const FogInfo::HomogenousRendering& v);
void from_json(const json& j, FogInfo::HomogenousRendering& v);
void to_json(json& j, const FogInfo& v);
void from_json(const json& j, FogInfo& v);
}