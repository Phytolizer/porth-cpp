#pragma once

#include <stdexcept>
namespace porth {

struct ParseError final : std::runtime_error {
    explicit ParseError(const std::string& what);
};

} // namespace porth
