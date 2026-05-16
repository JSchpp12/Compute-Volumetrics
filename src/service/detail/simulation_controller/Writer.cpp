#include "service/detail/simulation_controller/Writer.hpp"

#include "FogInfo_json.hpp"

#include <starlight/core/logging/LoggingFactory.hpp>

#include <fstream>

using nlohmann::json;

namespace service::simulation_controller
{

static void WriteDataToFile(const std::filesystem::path &path, const SimulationBounds &bounds)
{
    json startData;
    to_json(startData, bounds.start);

    json stopData;
    to_json(stopData, bounds.stop);

    json fullData;
    fullData["startData"] = startData;
    fullData["stopData"] = stopData;
    fullData["numSteps"] = bounds.numSteps; 

    std::ofstream ostream(path, std::ios::binary);
    ostream << std::setw(4) << fullData << std::endl;
}

int Writer::operator()(const std::filesystem::path &path)
{
    assert(m_bounds && "Bounds data was not properly initialized"); 

    WriteDataToFile(path, *m_bounds);
    return 0;
}
} // namespace controller::simulation_bounds_file