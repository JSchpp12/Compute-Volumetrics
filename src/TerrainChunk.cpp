#include "TerrainChunk.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "ConfigFile.hpp"
#include "FileHelpers.hpp"
#include "ManagerController_RenderResource_IndicesInfo.hpp"
#include "ManagerController_RenderResource_TextureFile.hpp"
#include "ManagerController_RenderResource_VertInfo.hpp"
#include "TransferRequest_VertInfo.hpp"
#include "TransferRequest_IndicesInfo.hpp"
#include "ManagerRenderResource.hpp"
#include "MathHelpers.hpp"
#include "TextureMaterial.hpp"
#include "Vertex.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <stdexcept>

TerrainChunk::TerrainChunk(const std::string &fullHeightFile, const std::string &nTextureFile,
                           const glm::dvec2 &northEast, const glm::dvec2 &southEast, const glm::dvec2 &southWest,
                           const glm::dvec2 &northWest, const glm::dvec3 &offset, const glm::dvec2 &center)
    : fullHeightFile(fullHeightFile), northEast(northEast), southEast(southEast), southWest(southWest),
      northWest(northWest), offset(offset), center(center)
{
    std::string terrainDir = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/terrains";
    std::optional<std::string> matchedFile =
        star::file_helpers::FindFileInDirectoryWithSameNameIgnoreFileType(terrainDir, nTextureFile);
    assert(matchedFile.has_value() && "Unable to find matching texture file");

    this->textureFile = matchedFile.value();
}

double TerrainChunk::getCenterHeightFromGDAL(const std::string &geoTiff, const glm::dvec2 &centerLatLon)
{
    GDALDataset *dataset = (GDALDataset *)GDALOpen(geoTiff.c_str(), GA_ReadOnly);

    if (dataset == NULL)
    {
        throw std::runtime_error("Failed to create dataset");
    }

    float *line = nullptr;

    GDALRasterBand *band = dataset->GetRasterBand(1);

    int nXSize = band->GetXSize();
    int nYSize = band->GetYSize();
    line = (float *)CPLMalloc(sizeof(float) * nXSize * nYSize);
    band->RasterIO(GF_Read, 0, 0, nXSize, nYSize, line, nXSize, nYSize, GDT_Float32, 0, 0);

    double result = line[0];

    CPLFree(line);
    GDALClose(dataset);

    return result;
}

void TerrainChunk::load()
{
    TerrainDataset dataset = TerrainDataset(this->fullHeightFile, this->northEast, this->southEast, this->southWest,
                                            this->northWest, this->center, this->offset);

    loadGeomInfo(dataset, verts, inds, this->firstLine, this->lastLine);
}

std::unique_ptr<star::StarMesh> TerrainChunk::getMesh(star::core::device::DeviceContext &context,
                                                      std::shared_ptr<star::StarMaterial> myMaterial)
{
    const auto graphicsIndex = context.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(); 

    const auto vertSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    star::Handle vertBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(vertSemaphore)->semaphore,
        std::make_unique<star::TransferRequest::VertInfo>(graphicsIndex, verts));

    const auto indSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    star::Handle indBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(indSemaphore)->semaphore,
        std::make_unique<star::TransferRequest::IndicesInfo>(graphicsIndex, inds));
    return std::make_unique<star::StarMesh>(vertBuffer, indBuffer, verts, inds, myMaterial, false);
}

std::string &TerrainChunk::getTextureFile()
{
    return this->textureFile;
}

void TerrainChunk::loadLocation(TerrainDataset &dataset, std::vector<glm::dvec3> &vertPositions,
                                std::vector<glm::vec2> &vertTextureCoords, std::vector<glm::dvec3> &firstLine,
                                std::vector<glm::dvec3> &lastLine)
{
    double xTexStep = 1.0f / (double)(dataset.getPixSize().x - 1);
    double yTexStep = 1.0f / (double)(dataset.getPixSize().y - 1);

    const glm::dvec2 horzLine_north = dataset.getNorthEast() - dataset.getNorthWest();
    const glm::dvec2 horzLine_south = dataset.getSouthEast() - dataset.getSouthWest();
    const glm::dvec2 vertLine_west = dataset.getSouthWest() - dataset.getNorthWest();
    const glm::dvec2 vertLine_east = dataset.getSouthEast() - dataset.getNorthEast();

    const double horzStep_north = (glm::length(horzLine_north) / (double)(dataset.getPixSize().x - 1));
    const double horzStep_south = (glm::length(horzLine_south) / (double)(dataset.getPixSize().x - 1));
    const double vertStep_west = (glm::length(vertLine_west) / (double)(dataset.getPixSize().y - 1));
    const double vertStep_east = (glm::length(vertLine_east) / (double)(dataset.getPixSize().y - 1));

    const glm::dvec2 horzLineDir_north = glm::normalize(horzLine_north);
    const glm::dvec2 horzLineDir_south = glm::normalize(horzLine_south);
    const glm::dvec2 vertLineDir_west = glm::normalize(vertLine_west);
    const glm::dvec2 vertLineDir_east = glm::normalize(vertLine_east);

    // calculate locations
    for (int i = 0; i < dataset.getPixSize().y; i++)
    {
        // problem is here
        const glm::dvec2 bordPosWest = dataset.getNorthWest() + (vertLineDir_west * vertStep_west * (double)i);
        const glm::dvec2 bordPosEast = dataset.getNorthEast() + (vertLineDir_east * vertStep_east * (double)i);

        for (int j = 0; j < dataset.getPixSize().x; j++)
        {
            const glm::dvec2 bordPosNorth = dataset.getNorthWest() + (horzLineDir_north * horzStep_north * (double)j);
            const glm::dvec2 bordPosSouth = dataset.getSouthWest() + (horzLineDir_south * horzStep_south * (double)j);

            // find intersection of two
            const glm::dvec2 intersection =
                calcIntersection(Line{bordPosNorth, bordPosSouth}, Line{bordPosWest, bordPosEast});

            vertPositions.push_back(glm::dvec3{intersection.x, intersection.y,
                                               dataset.getElevationAtTexCoords(dataset.applyOffsetToTexCoords(
                                                   dataset.getTexCoordsFromLatLon(intersection)))});

            vertTextureCoords.push_back(glm::vec2(j * xTexStep, i * yTexStep));
        }
    }
}

void TerrainChunk::loadInds(TerrainDataset &dataset, std::vector<uint32_t> &inds)
{
    uint32_t indexCounter = 0;

    for (int i = 0; i < dataset.getPixSize().y; i++)
    {
        for (int j = 0; j < dataset.getPixSize().x; j++)
        {
            if (j % 2 == 1 && i % 2 == 1)
            {
                // this is a 'central' vert where drawing should be based around
                //
                // uppper left
                uint32_t center = indexCounter;
                uint32_t centerLeft = indexCounter - 1;
                uint32_t centerRight = indexCounter + 1;
                uint32_t upperLeft = indexCounter - 1 - dataset.getPixSize().x;
                uint32_t upperCenter = indexCounter - dataset.getPixSize().x;
                uint32_t upperRight = indexCounter - dataset.getPixSize().x + 1;
                uint32_t lowerLeft = indexCounter + dataset.getPixSize().x - 1;
                uint32_t lowerCenter = indexCounter + dataset.getPixSize().x;
                uint32_t lowerRight = indexCounter + dataset.getPixSize().x + 1;
                // 1
                inds.push_back(center);
                inds.push_back(upperLeft);
                inds.push_back(centerLeft);
                // 2
                inds.push_back(center);
                inds.push_back(upperCenter);
                inds.push_back(upperLeft);

                if (i == dataset.getPixSize().y - 1 && j != dataset.getPixSize().x - 1)
                {
                    // bottom piece
                    // can do 3,4,5,6,
                    // 7
                    inds.push_back(center);
                    inds.push_back(centerRight);
                    inds.push_back(upperRight);
                    // 8
                    inds.push_back(center);
                    inds.push_back(upperRight);
                    inds.push_back(upperCenter);
                }
                else if (i != dataset.getPixSize().y - 1 && j == dataset.getPixSize().x - 1)
                {
                    // side piece
                    // cant do 5,6,7,8
                    // 3
                    inds.push_back(center);
                    inds.push_back(centerLeft);
                    inds.push_back(lowerLeft);
                    // 4
                    inds.push_back(center);
                    inds.push_back(lowerLeft);
                    inds.push_back(lowerCenter);
                }
                else if (i != dataset.getPixSize().y - 1 && j != dataset.getPixSize().x - 1)
                {
                    // 3
                    inds.push_back(center);
                    inds.push_back(upperRight);
                    inds.push_back(upperCenter);
                    // 7
                    inds.push_back(center);
                    inds.push_back(centerRight);
                    inds.push_back(upperRight);
                    // 5
                    inds.push_back(center);
                    inds.push_back(lowerRight);
                    inds.push_back(centerRight);
                    // 6
                    inds.push_back(center);
                    inds.push_back(lowerCenter);
                    inds.push_back(lowerRight);
                    // 7
                    inds.push_back(center);
                    inds.push_back(lowerLeft);
                    inds.push_back(lowerCenter);
                    // 8
                    inds.push_back(center);
                    inds.push_back(centerLeft);
                    inds.push_back(lowerLeft);
                }
            }
            indexCounter++;
        }
    }
}

void TerrainChunk::calculateNormals(std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds)
{
    // calculate normals
    for (int i = 0; i < inds.size(); i += 3)
    {
        auto &vert1 = verts.at(inds.at(i));
        auto &vert2 = verts.at(inds.at(i + 1));
        auto &vert3 = verts.at(inds.at(i + 2));

        glm::vec3 edge1 = vert2.pos - vert1.pos;
        glm::vec3 edge2 = vert3.pos - vert1.pos;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        vert1.normal += normal;
        vert2.normal += normal;
        vert3.normal += normal;
    }
}

void TerrainChunk::centerAroundTerrainOrigin(std::vector<glm::dvec3> &vertPositions,
                                             const glm::dvec3 &worldCenterLatLon) const
{
    const glm::dvec3 worldCenterECEF =
        MathHelpers::toECEF(worldCenterLatLon.x, worldCenterLatLon.y, worldCenterLatLon.z);
    const auto worldCenterToENUTransformation =
        MathHelpers::getECEFToENUTransformation(worldCenterLatLon.x, worldCenterLatLon.y);

    for (int i = 0; i < vertPositions.size(); i++)
    {
        const glm::dvec3 vertECEF = MathHelpers::toECEF(vertPositions[i].x, vertPositions[i].y, vertPositions[i].z);

        const glm::dvec3 displacedECEF = vertECEF - worldCenterECEF;
        const glm::dvec3 result = worldCenterToENUTransformation * displacedECEF;

        vertPositions[i] = glm::vec3{result.y, result.z, result.x};
    }
}

void TerrainChunk::loadGeomInfo(TerrainDataset &dataset, std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds,
                                std::vector<glm::dvec3> &firstLine, std::vector<glm::dvec3> &lastLine) const
{
    std::vector<glm::dvec3> rawVertPositionCoords = std::vector<glm::dvec3>();
    std::vector<glm::vec2> vertTextureCoords = std::vector<glm::vec2>();

    loadLocation(dataset, rawVertPositionCoords, vertTextureCoords, firstLine, lastLine);

    loadInds(dataset, inds);

    centerAroundTerrainOrigin(rawVertPositionCoords, dataset.getOffset());

    for (size_t i = 0; i < rawVertPositionCoords.size(); i++)
    {
        verts.push_back(star::Vertex(rawVertPositionCoords.at(i), {}, {}, vertTextureCoords.at(i)));
    }

    calculateNormals(verts, inds);
}

glm::dvec2 TerrainChunk::calcStep(const glm::dvec2 &startPoint, const glm::dvec2 &horizontalDirection,
                                  const double &horizontalStepSize, const glm::dvec2 &verticalDirection,
                                  const double &verticalStepSize, const int &stepsX, const int &stepsY)
{
    return glm::dvec2{startPoint + (horizontalDirection * horizontalStepSize * double(stepsX)) +
                      (verticalDirection * verticalStepSize * double(stepsY))};
}

glm::dvec2 TerrainChunk::calcIntersection(const Line &lineA, const Line &lineB)
{
    const double calcX = (lineB.intercept - lineA.intercept) / (lineA.slope - lineB.slope);
    return glm::dvec2{calcX, lineA.y(calcX)};
}

TerrainChunk::TerrainDataset::~TerrainDataset()
{
    try
    {
        if (this->gdalBuffer)
        {
            CPLFree(this->gdalBuffer);
            this->gdalBuffer = nullptr;
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Memory leak found" << std::endl;
    }
}

TerrainChunk::TerrainDataset::TerrainDataset(const std::string &path, const glm::dvec2 &northEast,
                                             const glm::dvec2 &southEast, const glm::dvec2 &southWest,
                                             const glm::dvec2 &northWest, const glm::dvec2 &center,
                                             const glm::dvec3 &offset)
    : path(path), northEast(northEast), southEast(southEast), southWest(southWest), northWest(northWest),
      center(center), offset(offset)
{
    if (!star::file_helpers::FileExists(path))
    {
        throw std::runtime_error("File does not exist");
    }

    GDALDataset *dataset = (GDALDataset *)GDALOpen(path.c_str(), GA_ReadOnly);
    if (dataset == NULL)
    {
        throw std::runtime_error("Failed to create dataset");
    }

    initTransforms(dataset);
    initPixelCoords(northEast, northWest, southEast);
    initBandSizes(dataset);
    initGDALBuffer(dataset);

    GDALClose(dataset);
}

glm::ivec2 TerrainChunk::TerrainDataset::getTexCoordsFromLatLon(const glm::dvec2 &latLon) const
{
    return glm::ivec2{static_cast<int>((latLon.y - geoTransforms[0]) / geoTransforms[1]),
                      static_cast<int>((latLon.x - geoTransforms[3]) / geoTransforms[5])};
}

glm::ivec2 TerrainChunk::TerrainDataset::applyOffsetToTexCoords(const glm::ivec2 &texCoords) const
{
    return glm::ivec2{texCoords.x - this->pixOffset.x + this->pixBorderSize,
                      texCoords.y - this->pixOffset.y + this->pixBorderSize};
}

float TerrainChunk::TerrainDataset::getElevationAtTexCoords(const glm::ivec2 &texCoords) const
{
    const int readIndex = texCoords.y * (this->pixSize.x + (2 * this->pixBorderSize)) + texCoords.x;
    float height = this->gdalBuffer[readIndex];

    return height;
}

void TerrainChunk::TerrainDataset::initTransforms(GDALDataset *dataset)
{
    if (GDALGetGeoTransform(dataset, this->geoTransforms) != CPLE_None)
    {
        throw std::runtime_error("Failed to obtain proper geotransform");
    }
}

void TerrainChunk::TerrainDataset::initPixelCoords(const glm::dvec2 &northEast, const glm::dvec2 &northWest,
                                                   const glm::dvec2 &southEast)
{
    const glm::ivec2 tNorthEast = getTexCoordsFromLatLon(northEast);
    const glm::ivec2 tNorthWest = getTexCoordsFromLatLon(northWest);
    const glm::ivec2 tSouthEast = getTexCoordsFromLatLon(southEast);
    const glm::ivec2 tSouthWest = getTexCoordsFromLatLon(southWest);

    const auto crossA = glm::ivec2{std::abs(tSouthEast.x - tNorthWest.x), std::abs(tSouthEast.y - tNorthWest.y)};
    const auto crossB = glm::ivec2{std::abs(tSouthWest.x - tNorthEast.x), std::abs(tSouthWest.y - tNorthEast.y)};
    this->pixSize = glm::ivec2{std::floor((crossA.x + crossB.x) / 2), std::floor((crossA.y + crossB.y) / 2)};

    this->pixOffset = tNorthWest;
    this->maxPixBounds = this->pixSize + this->pixOffset;
}

void TerrainChunk::TerrainDataset::initBandSizes(GDALDataset *dataset)
{
    GDALRasterBand *band = dataset->GetRasterBand(1);
    this->fullPixSize = glm::ivec2{band->GetXSize(), band->GetYSize()};
}

void TerrainChunk::TerrainDataset::initGDALBuffer(GDALDataset *dataset)
{
    this->gdalBuffer = (float *)CPLMalloc(sizeof(float) * (this->pixSize.x + 2 * this->pixBorderSize) *
                                          (this->pixSize.y + 2 * this->pixBorderSize));

    GDALRasterBand *band = dataset->GetRasterBand(1);
    CPLErr error = band->RasterIO(
        GF_Read, this->pixOffset.x - this->pixBorderSize, this->pixOffset.y - this->pixBorderSize,
        this->pixSize.x + (2 * this->pixBorderSize), this->pixSize.y + (2 * this->pixBorderSize), this->gdalBuffer,
        this->pixSize.x + (2 * this->pixBorderSize), this->pixSize.y + (2 * this->pixBorderSize), GDT_Float32, 0, 0);

    if (error != CE_None)
        throw std::runtime_error("Failed to read raster band");
}