#include "Application.hpp"

using namespace star;

Application::Application(star::StarScene& scene) : StarApplication(scene) {}

void Application::Load()
{
    this->camera.setPosition(glm::vec3{ -2.0, 1.0f, -2.0f });
    auto camPosition = this->camera.getPosition();
    this->camera.setLookDirection(-camPosition);

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto lionPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/lion-statue/source/rapid.obj";
    auto materialsPath = mediaDirectoryPath + "models/lion-statue/source";
    auto plantPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/aloevera/aloevera.obj";

    auto lion = BasicObject::New(lionPath);
    auto plant = BasicObject::New(plantPath);
    auto& lion_i = lion->createInstance();
    lion_i.setScale(glm::vec3{ 0.04f, 0.04f, 0.04f });
    lion_i.setPosition(glm::vec3{ 0.0, 0.0, 0.0 });
    lion_i.rotateGlobal(star::Type::Axis::x, -90);
    lion_i.moveRelative(glm::vec3{ 0.0, -1.0, 0.0 });

    auto& p_i = plant->createInstance();
    p_i.setPosition(glm::vec3{ -0.8, 0.0, 0.0 });

    this->scene.add(std::move(lion));
    this->scene.add(std::move(plant));

    this->scene.add(std::make_unique<star::Light>(star::Type::Light::directional, glm::vec3{ 10, 10, 10 }));

    loadFogModel();
}

void Application::Update()
{
}

void Application::onKeyPress(int key, int scancode, int mods)
{
}

void Application::onKeyRelease(int key, int scancode, int mods)
{
}

void Application::onMouseMovement(double xpos, double ypos)
{
}

void Application::onMouseButtonAction(int button, int action, int mods)
{
}

void Application::onScroll(double xoffset, double yoffset)
{
}

void Application::onWorldUpdate()
{
}

void Application::loadFogModel() {
    openvdb::initialize();
    // Create a shared pointer to a newly-allocated grid of a built-in type:
    // in this case, a FloatGrid, which stores one single-precision floating point
    // value per voxel.  Other built-in grid types include BoolGrid, DoubleGrid,
    // Int32Grid and Vec3SGrid (see openvdb.h for the complete list).
    // The grid comprises a sparse tree representation of voxel data,
    // user-supplied metadata and a voxel space to world space transform,
    // which defaults to the identity transform.
    openvdb::FloatGrid::Ptr grid =
        openvdb::FloatGrid::create(/*background value=*/2.0);
    // Populate the grid with a sparse, narrow-band level set representation
    // of a sphere with radius 50 voxels, located at (1.5, 2, 3) in index space.
    makeSphere(*grid, /*radius=*/50.0, /*center=*/openvdb::Vec3f(1.5, 2, 3));
    // Associate some metadata with the grid.
    grid->insertMeta("radius", openvdb::FloatMetadata(50.0));
    // Associate a scaling transform with the grid that sets the voxel size
    // to 0.5 units in world space.
    grid->setTransform(
        openvdb::math::Transform::createLinearTransform(/*voxel size=*/0.5));
    // Identify the grid as a level set.
    grid->setGridClass(openvdb::GRID_LEVEL_SET);
    // Name the grid "LevelSetSphere".
    grid->setName("LevelSetSphere");
    // Create a VDB file object.
    openvdb::io::File file("mygrids.vdb");
    // Add the grid pointer to a container.
    openvdb::GridPtrVec grids;
    grids.push_back(grid);
    // Write out the contents of the container.
    file.write(grids);
    file.close();

}