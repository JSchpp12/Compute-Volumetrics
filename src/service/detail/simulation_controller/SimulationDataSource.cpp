#include "service/detail/simulation_controller/SimulationDataSource.hpp"

#include "service/detail/simulation_controller/Reader.hpp"

#include <starlight/command/FileIO/ReadFromFile.hpp>
#include <starlight/core/CommandBus.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/job/tasks/IOTask.hpp>

namespace service::simulation_controller
{
SimulationDataSource SimulationDataSource::Builder::build()
{
    Reader reader{};
    auto future = reader.getFuture();

    auto payload = star::job::tasks::io::ReadPayload<Reader>{.filePath = m_path, .readFunction = std::move(reader)};
    auto readTask = star::job::tasks::io::CreateReadTask(std::move(payload));
    auto readCommand = star::command::file_io::ReadFromFile(std::move(readTask));
    m_cmdBus->submit(readCommand);

    return SimulationDataSource{PendingSimulationData{std::move(future)}};
}

SimulationData &SimulationDataSource::get()
{
    if (auto *pending = std::get_if<PendingSimulationData>(&m_data))
    {
        try
        {
            m_data = pending->get();
        }
        catch (...)
        {
            STAR_THROW("Attempted to call get on future that has already been consumed. This signifies that the json "
                       "file is not valid");
        }
    }

    return std::get<SimulationData>(m_data);
}

const SimulationData &SimulationDataSource::get() const
{
    return std::get<SimulationData>(m_data);
}
} // namespace service::simulation_controller
