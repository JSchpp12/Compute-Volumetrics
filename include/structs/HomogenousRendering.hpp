#pragma once

struct HomogenousRendering
{
    uint32_t maxNumSteps = 256;

    HomogenousRendering() = default;
    explicit HomogenousRendering(uint32_t maxNumSteps) : maxNumSteps(std::move(maxNumSteps))
    {
    }

    bool operator!=(const HomogenousRendering &other) const
    {
        return this->maxNumSteps != other.maxNumSteps;
    }

    bool operator==(const HomogenousRendering &other) const
    {
        return this->maxNumSteps == other.maxNumSteps;
    }

    const uint32_t &getMaxNumSteps() const
    {
        return maxNumSteps;
    }
    void setMaxNumSteps(uint32_t numSteps)
    {
        maxNumSteps = std::move(numSteps);
    }
};