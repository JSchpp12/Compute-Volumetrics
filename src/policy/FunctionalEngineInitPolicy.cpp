#include "policy/FunctionalEngineInitPolicy.hpp"

#include "service/ImageMetricManager.hpp"

star::core::device::StarDevice FunctionalEngineInitPolicy::createNewDevice(
    star::core::RenderingInstance &renderingInstance, std::set<star::Rendering_Features> &engineRenderingFeatures,
    std::set<star::Rendering_Device_Features> &engineRenderingDeviceFeatures)
{
    auto builder = star::core::device::StarDevice::Builder(renderingInstance)
                       .setRenderingDeviceFeatures(engineRenderingDeviceFeatures)
                       .setRenderingFeatures(engineRenderingFeatures);

    int selectedOverride = m_overrideDeviceIndex.has_value()
                               ? m_overrideDeviceIndex.value()
                               : star::ConfigFile::getInt(star::Config_Settings::required_device_feature_gpu_index, -1);
    if (selectedOverride != -1)
        builder.setOverrideDeviceID(selectedOverride);

    return builder.build();
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
    return star::service::Service{service::ImageMetricManager()};
}