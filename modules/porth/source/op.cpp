#include "porth/op.hpp"

#include "iota_generated/op_id.hpp"

porth::Op::Op(const OpId id, std::string filePath, const std::size_t lineNumber, const std::size_t columnNumber)
    : Op(id, std::move(filePath), lineNumber, columnNumber, 0) {
}

porth::Op::Op(
    const OpId id,
    std::string filePath,
    const std::size_t lineNumber,
    const std::size_t columnNumber,
    const std::int64_t operand)
    : id(id), filePath(std::move(filePath)), lineNumber(lineNumber), columnNumber(columnNumber), operand(operand) {
}

std::ostream& operator<<(std::ostream& os, const porth::Op& op) {
    return os << op.id.name;
}
