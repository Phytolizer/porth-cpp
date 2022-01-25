#pragma once

#include <stdexcept>
namespace porth {

struct SimulationError : std::runtime_error {
    explicit SimulationError(const std::string& what);
};

} // namespace porth
