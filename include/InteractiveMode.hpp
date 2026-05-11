#pragma once

#include "structs/AppConfig.hpp"

#include <filesystem>

#ifdef STAR_ENABLE_PRESENTATION

class InteractiveMode
{
  public:
    int run(std::unique_ptr<AppConfig> cfg);
};

#endif