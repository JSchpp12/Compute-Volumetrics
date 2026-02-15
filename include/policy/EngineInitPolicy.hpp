#pragma once

#include <starlight/policy/DefaultEngineInitPolicy.hpp>

class EngineInitPolicy : public star::policy::DefaultEngineInitPolicy
{
  public:
    EngineInitPolicy() = default;

  protected:
    virtual std::vector<star::service::Service> addAdditionalServices() override;

    star::service::Service CreateImageMetricManagerService() const;
};