#include "porth/parse_error.hpp"

porth::ParseError::ParseError(const std::string& what) : std::runtime_error(what) {
}
