#include "service/controller/detail/simulation_bounds_file/Writer.hpp"

#include "fog_info/JsonUtils.hpp"

#include <starlight/core/logging/LoggingFactory.hpp>

#include <fstream>

using nlohmann::json;

namespace controller::simulation_bounds_file
{

static void WriteDataToFile(const std::string &path, const SimulationBounds &bounds)
{
    json startData;
    fog_info::to_json(startData, bounds.start);

    json stopData;
    fog_info::to_json(stopData, bounds.stop);

    json fullData;
    fullData["startData"] = startData;
    fullData["stopData"] = stopData;
    fullData["numSteps"] = bounds.numSteps; 

    std::ofstream ostream(path, std::ios::binary);
    ostream << std::setw(4) << fullData << std::endl;
}

int Writer::operator()(const std::string &path)
{
    WriteDataToFile(path, m_bounds);
    return 0;
}
} // namespace controller::simulation_bounds_file