#include "porth/sim.hpp"

#include "porth/mem.hpp"
#include "porth/simulation_error.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <sstream>

template <typename T> T vecPop(std::vector<T>& v) {
    assert(!v.empty() && "vecPop: empty vector");
    T result = std::move(v.back());
    v.pop_back();
    return result;
}

void porth::simulateProgram(const std::vector<Op>& program, bool debugMode) {
    static_assert(OpIds::Count.discriminant == 34, "Exhaustive handling of OpIds in simulateProgram");
    std::vector<std::int64_t> stack;
    std::array<std::uint8_t, MEM_CAPACITY> mem{};
    // execution is not linear, so we use a for loop with an index
    for (size_t ip = 0; ip < program.size();) {
        if (const Op& op = program[ip]; op.id == OpIds::Push) {
            stack.push_back(op.operand);
            ++ip;
        } else if (op.id == OpIds::Plus) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a + b);
            ++ip;
        } else if (op.id == OpIds::Minus) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a - b);
            ++ip;
        } else if (op.id == OpIds::Eq) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a == b ? 1 : 0);
            ++ip;
        } else if (op.id == OpIds::Ne) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a != b ? 1 : 0);
            ++ip;
        } else if (op.id == OpIds::Gt) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a > b ? 1 : 0);
            ++ip;
        } else if (op.id == OpIds::Lt) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a < b ? 1 : 0);
            ++ip;
        } else if (op.id == OpIds::Ge) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a >= b ? 1 : 0);
            ++ip;
        } else if (op.id == OpIds::Le) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a <= b ? 1 : 0);
            ++ip;
        } else if (op.id == OpIds::If) {
            if (const std::int64_t a = vecPop(stack); a == 0) {
                // simulate a goto
                ip = static_cast<size_t>(op.operand);
            } else {
                ++ip;
            }
        } else if (op.id == OpIds::Else) {
            ip = static_cast<size_t>(op.operand);
        } else if (op.id == OpIds::End) {
            ip = static_cast<size_t>(op.operand);
        } else if (op.id == OpIds::Print) {
            std::cout << stack.back() << "\n";
            ++ip;
        } else if (op.id == OpIds::Dup) {
            const std::int64_t a = stack.back();
            stack.push_back(a);
            ++ip;
        } else if (op.id == OpIds::Dup2) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a);
            stack.push_back(b);
            stack.push_back(a);
            stack.push_back(b);
            ++ip;
        } else if (op.id == OpIds::Swap) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(b);
            stack.push_back(a);
            ++ip;
        } else if (op.id == OpIds::Drop) {
            stack.pop_back();
            ++ip;
        } else if (op.id == OpIds::While) {
            ++ip;
        } else if (op.id == OpIds::Do) {
            const std::int64_t a = vecPop(stack);
            if (a == 0) {
                ip = static_cast<size_t>(op.operand);
            } else {
                ++ip;
            }
        } else if (op.id == OpIds::Mem) {
            stack.push_back(0);
            ++ip;
        } else if (op.id == OpIds::Load) {
            const std::int64_t a = vecPop(stack);
            // Interpret a as a memory address.
            // Here be dragons.
            const auto addr = static_cast<std::size_t>(a);
            if (addr >= mem.size()) {
                std::ostringstream errorMessage;
                errorMessage << "load: invalid memory address " << addr;
                throw SimulationError(errorMessage.str());
            }
            const std::uint8_t b = mem[addr];
            stack.push_back(static_cast<std::int64_t>(b));
            ++ip;
        } else if (op.id == OpIds::Store) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            // Interpret a as a memory address.
            // Here be dragons.
            const auto addr = static_cast<std::size_t>(a);
            if (addr >= mem.size()) {
                std::ostringstream errorMessage;
                errorMessage << "store: invalid memory address " << addr;
                throw SimulationError(errorMessage.str());
            }
            mem[addr] = static_cast<std::uint8_t>(b);
            ++ip;
        } else if (op.id == OpIds::Syscall1) {
            throw SimulationError("syscall1: unimplemented");
        } else if (op.id == OpIds::Syscall2) {
            throw SimulationError("syscall2: unimplemented");
        } else if (op.id == OpIds::Syscall3) {
            const std::int64_t syscallNumber = vecPop(stack);
            const std::int64_t arg1 = vecPop(stack);
            const std::int64_t arg2 = vecPop(stack);
            const std::int64_t arg3 = vecPop(stack);
            if (syscallNumber == 1) {
                const std::int64_t fd = arg1;
                const std::int64_t buf = arg2;
                const std::int64_t count = arg3;
                const std::string_view s = {
                    reinterpret_cast<const char*>(&mem[buf]),
                    static_cast<std::size_t>(count),
                };
                if (fd == 1) {
                    std::cout << s;
                } else if (fd == 2) {
                    std::cerr << s;
                } else {
                    std::ostringstream errorMessage;
                    errorMessage << "syscall3: unknown file descriptor " << fd;
                    throw SimulationError(errorMessage.str());
                }
            } else {
                std::ostringstream errorMessage;
                errorMessage << "syscall3: unknown syscall " << syscallNumber;
                throw SimulationError(errorMessage.str());
            }
            ++ip;
        } else if (op.id == OpIds::Syscall4) {
            throw SimulationError("syscall4: unimplemented");
        } else if (op.id == OpIds::Syscall5) {
            throw SimulationError("syscall5: unimplemented");
        } else if (op.id == OpIds::Syscall6) {
            throw SimulationError("syscall6: unimplemented");
        } else if (op.id == OpIds::Shr) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a >> b);
            ++ip;
        } else if (op.id == OpIds::Shl) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a << b);
            ++ip;
        } else if (op.id == OpIds::Bor) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a | b);
            ++ip;
        } else if (op.id == OpIds::Band) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a & b);
            ++ip;
        } else if (op.id == OpIds::Over) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a);
            stack.push_back(b);
            stack.push_back(a);
            ++ip;
        } else if (op.id == OpIds::Mod) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a % b);
            ++ip;
        }
    }
    if (debugMode) {
        std::cout << "[INFO] Memory dump\n";
        const std::string_view s{reinterpret_cast<const char*>(mem.data()), 20};
        std::cout << s << "\n";
    }
}
