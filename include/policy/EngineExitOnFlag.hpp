#pragma once

#include <memory>

class EngineExitOnFlag
{
  public:
    explicit EngineExitOnFlag(std::shared_ptr<bool> flag);

    bool shouldExit(); 
  private:
    std::shared_ptr<bool> m_flag;
};