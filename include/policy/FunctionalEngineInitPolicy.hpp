#pragma once

#include <starlight/policy/DefaultEngineInitPolicy.hpp>

#include <functional>

class FunctionalEngineInitPolicy : public star::policy::DefaultEngineInitPolicy
{
  public:
    explicit FunctionalEngineInitPolicy(std::function<std::vector<star::service::Service>(void)> addServicesFun);

  protected:
    std::function<std::vector<star::service::Service>(void)> m_addServicesFun;

    virtual std::vector<star::service::Service> addAdditionalServices() override;

    star::service::Service CreateImageMetricManagerService() const;
};