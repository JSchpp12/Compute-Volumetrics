#pragma once

#ifndef STAR_ENABLE_PRESENTATION
#include <starlight/service/FrameInFlightControllerService.hpp>
class HeadlessMode
{
  public:
    int run(std::string terrainPath, std::string controllerPath);

  private:
};
#endif