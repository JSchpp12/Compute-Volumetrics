#pragma once

#include <nlohmann/json.hpp>

class FogInfo;

void to_json(nlohmann::json &j, const ::FogInfo::LinearFogInfo &v);
void from_json(const nlohmann::json &j, ::FogInfo::LinearFogInfo &v);
void to_json(nlohmann::json &j, const ::FogInfo::ExpFogInfo &v);
void from_json(const nlohmann::json &j, ::FogInfo::ExpFogInfo &v);
void to_json(nlohmann::json &j, const ::FogInfo::MarchedFogInfo &v);
void from_json(const nlohmann::json &j, ::FogInfo::MarchedFogInfo &v);
void to_json(nlohmann::json &j, const ::FogInfo::HomogenousRendering &v);
void from_json(const nlohmann::json &j, ::FogInfo::HomogenousRendering &v);
void to_json(nlohmann::json &j, const ::FogInfo &v);
void from_json(const nlohmann::json &j, ::FogInfo &v);