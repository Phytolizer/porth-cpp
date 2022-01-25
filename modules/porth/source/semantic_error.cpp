#include "porth/semantic_error.hpp"

porth::SemanticError::SemanticError(const std::string& what) : runtime_error(what) {
}
