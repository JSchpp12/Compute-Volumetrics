#include <iostream>

#include "StarEngine.hpp"

#include "Application.hpp"

#include "modules/InteractionSystem.hpp"
#include "common/ConfigFile.hpp"

#include "managers/ShaderManager.hpp"
#include "managers/ObjectManager.hpp"
#include "managers/TextureManager.hpp"
#include "managers/LightManager.hpp"
#include "managers/MapManager.hpp"

#include "BasicCamera.hpp"

const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 1200;

#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"

int main() {

    //TODO: give object and camera and lights to engine then pass ref to application -- will fix deletion problem
    auto engine = star::StarEngine();
    auto application = Application(engine.getScene());
    application.Load();
    engine.init(application);

    try {
        engine.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        do {
            std::cout << "Press a key to exit..." << std::endl;
        } while (std::cin.get() != '\n');
        return EXIT_FAILURE;
    }
}