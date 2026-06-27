#pragma once

#ifndef STAR_ENABLE_PRESENTATION
#include "config/AppConfigInfo.hpp"

#include <starlight/service/FrameInFlightControllerService.hpp>

class HeadlessMode
{
  public:
    int run(std::unique_ptr<config::AppConfigInfo> cfg);

  private:
};
#endif