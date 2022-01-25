#pragma once

#include <stdexcept>

namespace porth {

struct SemanticError final : std::runtime_error {
    explicit SemanticError(const std::string& what);
};

} // namespace porth
