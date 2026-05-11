#pragma once

#ifndef STAR_ENABLE_PRESENTATION
#include "structs/AppConfig.hpp"

#include <starlight/service/FrameInFlightControllerService.hpp>

class HeadlessMode
{
  public:
    int run(std::unique_ptr<AppConfig> cfg);

  private:
};
#endif