#include "config/AppConfigInfo_json.hpp"
#include "config/AppConfigInfo.hpp"
#include "config/InteractiveConfigInfo.hpp"
#include "config/InteractiveConfigInfo_json.hpp"

namespace config
{
void to_json(nlohmann::json &j, const AppConfigInfo &v)
{
    j = nlohmann::json{{"volumeName", v.volumeName},
                       {"terrainDir", v.terrainDir},
                       {"simControllerPath", v.simControllerPath},
                       {"enableDistanceMarkers", v.enableDistanceMarkers},
                       {"enableCutoffHighlighting", v.enableCutoffHighlighting}};

    if (v.overrideRenderingDevice.has_value())
        j["overrideRenderingDevice"] = *v.overrideRenderingDevice;

    nlohmann::json interactiveData;
    to_json(interactiveData, v.interactiveConfig);
    j["interactiveConfig"] = std::move(interactiveData);
}

void from_json(const nlohmann::json &j, AppConfigInfo &v)
{
    v.volumeName = j.value("volumeName", v.volumeName);
    v.terrainDir = j.value("terrainDir", v.terrainDir);
    v.simControllerPath = j.value("simControllerPath", v.simControllerPath);
    v.enableDistanceMarkers = j.value("enableDistanceMarkers", v.enableDistanceMarkers);
    v.enableCutoffHighlighting = j.value("enableCutoffHighlighting", v.enableCutoffHighlighting);

    if (j.contains("overrideRenderingDevice"))
        v.overrideRenderingDevice = j["overrideRenderingDevice"].get<int>();

    if (j.contains("interactiveConfig"))
        from_json(j["interactiveConfig"], v.interactiveConfig);
}
} // namespace config