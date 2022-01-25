#pragma once

#include <cstdint>
#include <iota_generated/op_id.hpp>
#include <ostream>

namespace porth {

struct Op {
    OpId id;
    std::string filePath;
    std::size_t lineNumber;
    std::size_t columnNumber;
    std::int64_t operand;

    Op(OpId id, std::string filePath, std::size_t lineNumber, std::size_t columnNumber);
    Op(OpId id, std::string filePath, std::size_t lineNumber, std::size_t columnNumber, std::int64_t operand);
};

} // namespace porth

std::ostream& operator<<(std::ostream& os, const porth::Op& op);
