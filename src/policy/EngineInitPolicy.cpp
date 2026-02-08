#include "policy/EngineInitPolicy.hpp"

#include "service/ImageMetricManager.hpp"

std::vector<star::service::Service> EngineInitPolicy::addAdditionalServices()
{
    auto s = std::vector<star::service::Service>(1);
    s[0] = CreateImageMetricManagerService();

    return s;
}

star::service::Service EngineInitPolicy::CreateImageMetricManagerService() const
{
    return star::service::Service{ImageMetricManager()};
}