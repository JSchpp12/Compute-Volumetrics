#pragma once

#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <tbb/tbb.h>

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "StarBuffers/Buffer.hpp"
#include "device/StarDevice.hpp"
#include "StarMesh.hpp"

class TerrainChunk
{
  public:
    TerrainChunk(const std::string &fullHeightFile, const std::string &nTextureFile, const glm::dvec2 &northEast,
                 const glm::dvec2 &southEast, const glm::dvec2 &southWest, const glm::dvec2 &northWest,
                 const glm::dvec3 &offset, const glm::dvec2 &center);

    /// @brief Load meshes from the provided files
    void load();

    std::unique_ptr<star::StarMesh> getMesh(star::core::device::DeviceContext &context, std::shared_ptr<star::StarMaterial> myMaterial);

    std::string &getTextureFile();

    star::StarBuffers::Buffer &getIndexBuffer()
    {
        assert(this->indBuffer && "Index buffer has not been initialized. Make sure to call load() "
                                  "first.");
        return *this->indBuffer;
    }

    star::StarBuffers::Buffer &getVertexBuffer()
    {
        assert(this->vertBuffer && "Vertex buffer has not been initialized. Make sure to call load() "
                                   "first.");
        return *this->vertBuffer;
    }

    static double getCenterHeightFromGDAL(const std::string &geoTiff, const glm::dvec2 &centerLatLon);

    std::vector<glm::dvec3> lastLine;
    std::vector<glm::dvec3> firstLine;

  private:
    struct Line
    {
        const double slope, intercept;

        Line(const glm::dvec2 &pointA, const glm::dvec2 &pointB)
            : slope((pointB.y - pointA.y) / (pointB.x - pointA.x)), intercept(pointA.y - (slope * pointA.x))
        {
        }

        double y(const double &x) const
        {
            return ((this->slope * x) + this->intercept);
        }
    };

    class TerrainDataset
    {
      public:
        TerrainDataset(const std::string &path, const glm::dvec2 &northEast, const glm::dvec2 &southEast,
                       const glm::dvec2 &southWest, const glm::dvec2 &northWest, const glm::dvec2 &center,
                       const glm::dvec3 &offset);

        // no copy
        TerrainDataset(const TerrainDataset &) = delete;
        TerrainDataset &operator=(const TerrainDataset &) = delete;

        TerrainDataset(TerrainDataset &&) noexcept = default;
        // no move
        TerrainDataset &operator=(TerrainDataset &&) noexcept = delete;

        ~TerrainDataset();

        float getElevationAtTexCoords(const glm::ivec2 &texCoords) const;

        glm::ivec2 getTexCoordsFromLatLon(const glm::dvec2 &latLon) const;

        glm::ivec2 applyOffsetToTexCoords(const glm::ivec2 &texCoords) const;

        const glm::dvec2 &getNorthEast() const
        {
            return this->northEast;
        }
        const glm::dvec2 &getSouthEast() const
        {
            return this->southEast;
        }
        const glm::dvec2 &getSouthWest() const
        {
            return this->southWest;
        }
        const glm::dvec2 &getNorthWest() const
        {
            return this->northWest;
        }
        const glm::dvec3 &getOffset() const
        {
            return this->offset;
        }
        const glm::ivec2 &getPixSize() const
        {
            return this->pixSize;
        }
        const glm::dvec2 &getCenter() const
        {
            return this->center;
        }

      private:
        const std::string path;
        const glm::dvec2 northEast, southEast, southWest, northWest, center;
        const glm::dvec3 offset;
        const int pixBorderSize = 4;

        float *gdalBuffer = nullptr;
        glm::ivec2 fullPixSize, maxPixBounds, pixOffset, pixSize;
        double geoTransforms[6];

        void initTransforms(GDALDataset *dataset);

        void initBandSizes(GDALDataset *dataset);

        void initPixelCoords(const glm::dvec2 &northEast, const glm::dvec2 &northWest, const glm::dvec2 &southEast);

        void initGDALBuffer(GDALDataset *dataset);
    };

    std::vector<star::Vertex> verts;
    std::vector<uint32_t> inds;
    std::unique_ptr<star::StarBuffers::Buffer> indBuffer, vertBuffer;
    std::unique_ptr<star::StarMesh> mesh;
    std::string textureFile, fullHeightFile;
    const glm::dvec2 northEast, southEast, southWest, northWest, center;
    const glm::dvec3 offset;

    /**
     * @brief Extract height info from the file and calculate ver
     *
     * @param dataset GDALDataset to use
     * @param vertPositions
     * @param vertTextureCoords
     * @param firstLine
     * @param lastLine
     */
    static void loadLocation(TerrainDataset &dataset, std::vector<glm::dvec3> &vertPositions,
                             std::vector<glm::vec2> &vertTextureCoords, std::vector<glm::dvec3> &firstLine,
                             std::vector<glm::dvec3> &lastLine);

    static void loadInds(TerrainDataset &dataset, std::vector<uint32_t> &inds);

    static void calculateNormals(std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds);

    /// @brief Update all vert locations to be centered around the terrain center
    /// @param terrainCenter center of the terrain
    /// @param verts all of the vertices to update
    void centerAroundTerrainOrigin(std::vector<glm::dvec3> &vertPositions, const glm::dvec3 &worldCenterLatLon) const;

    void loadGeomInfo(TerrainDataset &dataset, std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds,
                      std::vector<glm::dvec3> &firstLine, std::vector<glm::dvec3> &lastLine) const;

    static glm::dvec2 calcStep(const glm::dvec2 &startPoint, const glm::dvec2 &horizontalDirection,
                               const double &horizontalStepSize, const glm::dvec2 &verticalDirection,
                               const double &verticalStepSize, const int &stepsX, const int &stepsY);

    static glm::dvec2 calcIntersection(const Line &lineA, const Line &lineB);
};

struct TerrainChunkProcessor
{
    TerrainChunkProcessor(TerrainChunk chunks[]) : chunks(chunks) {};

    void operator()(const tbb::blocked_range<size_t> &r) const
    {
        TerrainChunk *localChunks = this->chunks;

        for (size_t i = r.begin(); i != r.end(); ++i)
        {
            localChunks[i].load();
        }
    }

  private:
    TerrainChunk *const chunks;
};