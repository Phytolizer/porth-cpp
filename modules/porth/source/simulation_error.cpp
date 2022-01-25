#include "porth/simulation_error.hpp"

porth::SimulationError::SimulationError(const std::string& what) : std::runtime_error(what) {
}