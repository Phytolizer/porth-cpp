#pragma once

#include <stdexcept>
namespace porth {

struct SimulationError final : std::runtime_error {
    explicit SimulationError(const std::string& what);
};

} // namespace porth
