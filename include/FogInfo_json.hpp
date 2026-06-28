#pragma once

#include <nlohmann/json.hpp>

class FogInfo;
struct LinearFogInfo;
struct ExpFogInfo;
struct MarchedFogInfo;
struct HomogenousRendering;

void to_json(nlohmann::json &j, const LinearFogInfo &v);
void from_json(const nlohmann::json &j, LinearFogInfo &v);
void to_json(nlohmann::json &j, const ExpFogInfo &v);
void from_json(const nlohmann::json &j, ExpFogInfo &v);
void to_json(nlohmann::json &j, const MarchedFogInfo &v);
void from_json(const nlohmann::json &j, MarchedFogInfo &v);
void to_json(nlohmann::json &j, const HomogenousRendering &v);
void from_json(const nlohmann::json &j, HomogenousRendering &v);
void to_json(nlohmann::json &j, const FogInfo &v);
void from_json(const nlohmann::json &j, FogInfo &v);