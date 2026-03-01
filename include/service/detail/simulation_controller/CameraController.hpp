#pragma once

#include "service/detail/simulation_controller/camera_controller/Circle.hpp"

#include <starlight/virtual/StarCamera.hpp>

#include <nlohmann/json.hpp>

#include <optional>
#include <variant>

namespace service::simulation_controller
{
class CameraController
{
  public:
    CameraController() = default;
    explicit CameraController(camera_controller::Circle controller); 
    bool operator!() const;

    void tick(star::StarCamera &camera); 
    void reset(star::StarCamera &camera);
    bool isDone() const; 

  private:
    std::optional<std::variant<camera_controller::Circle>> m_controller = std::nullopt;
};
} // namespace service::simulation_controller