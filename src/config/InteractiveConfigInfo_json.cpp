#include "config/InteractiveConfigInfo_json.hpp"
#include "config/InteractiveConfigInfo.hpp"

namespace config
{
void to_json(nlohmann::json &j, const InteractiveConfigInfo &v)
{
    j = nlohmann::json{{"cameraMovementSpeed", v.cameraMovementSpeed},
                       {"cameraSensitivity", v.cameraSensitivity},
                       {"objectMovementSpeed", v.objectMovementSpeed}};
}

void from_json(const nlohmann::json &j, InteractiveConfigInfo &v)
{
    v.cameraMovementSpeed = j.value("cameraMovementSpeed", v.cameraMovementSpeed);
    v.cameraSensitivity = j.value("cameraSensitivity", v.cameraSensitivity);
    v.objectMovementSpeed = j.value("objectMovementSpeed", v.objectMovementSpeed);
}
} // namespace config