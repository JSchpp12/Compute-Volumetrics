#pragma once

#include "config/AppConfigInfo.hpp"

#include <filesystem>

#ifdef STAR_ENABLE_PRESENTATION

class InteractiveMode
{
  public:
    int run(std::unique_ptr<config::AppConfigInfo> cfg);
};

#endif