#include "AABBInfo.hpp"

void AABBInfo::writeBufferData(star::StarBuffer& buffer)
{
	buffer.map(); 

	std::array<glm::vec4, 2> aabbBounds = this->aabbBounds;
	buffer.writeToBuffer(aabbBounds.data(), sizeof(this->aabbBounds));

	buffer.unmap(); 
}
