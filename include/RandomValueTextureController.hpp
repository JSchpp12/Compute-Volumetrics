// #pragma once

// #include "RandomValueTexture.hpp"

// #include "ManagerController_RenderResource_Texture.hpp"

// class RandomValueTextureController : public star::ManagerController::RenderResource::Texture
// {
//   public:
//     RandomValueTextureController(uint32_t width, uint32_t height);
//     virtual ~RandomValueTextureController() = default; 

//     virtual std::unique_ptr<star::TransferRequest::Texture> createTransferRequest(star::core::device::StarDevice &device) override; 

//   private:
//     uint32_t m_width, m_height;
// };