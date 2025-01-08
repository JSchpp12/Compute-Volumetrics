#include "SampledVolumeTexture.hpp"

std::optional<std::unique_ptr<unsigned char>> SampledVolumeTexture::data()
{
	std::vector<unsigned char> data; 
	for (int i = 0; i < this->sampledData->size(); i++) {
		for (int j = 0; j < this->sampledData->at(i).size(); j++) {
			for (int k = 0; k < this->sampledData->at(i).at(j).size(); k++) {
				unsigned char packedData[sizeof(float)] = {};
				memcpy(packedData, &this->sampledData->at(i).at(j).at(k), sizeof(float));
				data.push_back(packedData[3]);
				data.push_back(packedData[2]);
				data.push_back(packedData[1]);
				data.push_back(packedData[0]);
			}
		}
	}
	auto prepData = std::unique_ptr<unsigned char>(new unsigned char[data.size()]);
	std::copy(data.begin(), data.end(), prepData.get()); 

	this->sampledData.reset(); 
	return std::move(prepData); 
}

int SampledVolumeTexture::getWidth()
{
	return this->width; 
}

int SampledVolumeTexture::getHeight()
{
	return this->height;
}

int SampledVolumeTexture::getChannels()
{
	return 1;
}

int SampledVolumeTexture::getDepth()
{
	return this->depth; 
}
