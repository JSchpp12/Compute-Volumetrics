#include "policy/EngineExitOnFlag.hpp"

#include <cassert>

EngineExitOnFlag::EngineExitOnFlag(std::shared_ptr<bool> flag) : m_flag(std::move(flag))
{
    assert(m_flag && *m_flag == false && "Flag should be valid and set to false initially");
}

bool EngineExitOnFlag::shouldExit()
{
    return *m_flag == true;
}