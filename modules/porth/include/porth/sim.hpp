#pragma once

#include "porth/op.hpp"

#include <vector>

namespace porth {

void simulateProgram(const std::vector<Op>& program, bool debugMode);

}
