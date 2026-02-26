#include "policy/FunctionalEngineInitPolicy.hpp"

#include "service/ImageMetricManager.hpp"
#include "service/controller/CircleCameraController.hpp"

FunctionalEngineInitPolicy::FunctionalEngineInitPolicy(
    std::function<std::vector<star::service::Service>(void)> addServicesFun)
    : m_addServicesFun(std::move(addServicesFun))
{
}

std::vector<star::service::Service> FunctionalEngineInitPolicy::addAdditionalServices()
{
    auto s = std::vector<star::service::Service>(1);
    s[0] = CreateImageMetricManagerService();

    if (m_addServicesFun)
    {
        auto funServices = m_addServicesFun();
        for (size_t i = 0; i < funServices.size(); i++)
        {
            s.emplace_back(std::move(funServices[i]));
        }
    }
    return s;
}

star::service::Service FunctionalEngineInitPolicy::CreateImageMetricManagerService() const
{
    return star::service::Service{ImageMetricManager()};
}