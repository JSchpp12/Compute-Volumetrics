#include "policy/EngineInitPolicy.hpp"

#include "service/ImageMetricManager.hpp"
#include "service/controller/CircleCameraController.hpp"

static star::service::Service CreateCameraControllerService()
{
    return star::service::Service{CircleCameraController{}};
}

std::vector<star::service::Service> EngineInitPolicy::addAdditionalServices()
{
    auto s = std::vector<star::service::Service>(2);
    s[0] = CreateImageMetricManagerService();
    s[1] = CreateCameraControllerService();
    return s;
}

star::service::Service EngineInitPolicy::CreateImageMetricManagerService() const
{
    return star::service::Service{ImageMetricManager()};
}