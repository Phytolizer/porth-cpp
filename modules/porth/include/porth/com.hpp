#pragma once

#include "porth/op.hpp"

#include <string>
#include <vector>

namespace porth {

int compileProgram(const std::vector<Op>& program, const std::string& outFilePath);

}
