#include "AABBInfo.hpp"

void AABBTransfer::writeData(star::StarBuffer& buffer) const{
	buffer.map(); 

	std::array<glm::vec4, 2> aabbBounds = this->aabbBounds;
	buffer.writeToBuffer(aabbBounds.data(), sizeof(this->aabbBounds));

	buffer.unmap(); 
}

std::unique_ptr<star::TransferRequest::Buffer> AABBController::createTransferRequest(const vk::PhysicalDevice& physicalDevice) {
	return std::make_unique<AABBTransfer>(this->aabbBounds);
}
