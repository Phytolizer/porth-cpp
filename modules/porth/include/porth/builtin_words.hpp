#pragma once

#include <array>
#include <iota_generated/op_id.hpp>

namespace porth {

static_assert(OpIds::Count.discriminant == 34, "Exhaustive handling of OpIds in BUILTIN_WORDS");
constexpr std::array BUILTIN_WORDS = {
    std::pair{"+", OpIds::Plus},
    std::pair{"-", OpIds::Minus},
    std::pair{"mod", OpIds::Mod},
    std::pair{"print", OpIds::Print},
    std::pair{"=", OpIds::Eq},
    std::pair{"!=", OpIds::Ne},
    std::pair{">", OpIds::Gt},
    std::pair{"<", OpIds::Lt},
    std::pair{">=", OpIds::Ge},
    std::pair{"<=", OpIds::Le},
    std::pair{"shr", OpIds::Shr},
    std::pair{"shl", OpIds::Shl},
    std::pair{"bor", OpIds::Bor},
    std::pair{"band", OpIds::Band},
    std::pair{"if", OpIds::If},
    std::pair{"end", OpIds::End},
    std::pair{"else", OpIds::Else},
    std::pair{"dup", OpIds::Dup},
    std::pair{"dup2", OpIds::Dup2},
    std::pair{"swap", OpIds::Swap},
    std::pair{"drop", OpIds::Drop},
    std::pair{"over", OpIds::Over},
    std::pair{"while", OpIds::While},
    std::pair{"do", OpIds::Do},
    std::pair{"mem", OpIds::Mem},
    std::pair{".", OpIds::Store},
    std::pair{",", OpIds::Load},
    std::pair{"syscall1", OpIds::Syscall1},
    std::pair{"syscall2", OpIds::Syscall2},
    std::pair{"syscall3", OpIds::Syscall3},
    std::pair{"syscall4", OpIds::Syscall4},
    std::pair{"syscall5", OpIds::Syscall5},
    std::pair{"syscall6", OpIds::Syscall6},
};

} // namespace porth
