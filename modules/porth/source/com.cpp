#include "porth/com.hpp"

#include "porth/mem.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

std::string labelName(const std::int64_t offset) {
    std::ostringstream result;
    result << "_porth_addr_" << offset;
    return result.str();
}

std::ostream& emit(std::ostream& output, const size_t indent) {
    for (size_t i = 0; i < indent; ++i) {
        output << "    ";
    }
    return output;
}

int porth::compileProgram(const std::vector<Op>& program, const std::string& outFilePath) {
    std::ofstream output{outFilePath};
    if (!output) {
        std::cerr << "[ERROR] failed to open '" << outFilePath << "' for writing\n";
        return 1;
    }
    size_t indent = 0;
    output << "#include <array>\n";
    output << "#include <cstdint>\n";
    output << "#include <iostream>\n";
    output << "#include <stack>\n";
    emit(output, indent) << "int main() {\n";
    ++indent;
    emit(output, indent) << "std::array<std::uint8_t, " << MEM_CAPACITY << "> mem;\n";
    emit(output, indent) << "std::stack<std::int64_t> _porth_stack;\n";
    static_assert(OpIds::Count.discriminant == 34, "Exhaustive handling of OpIds in compileProgram");
    for (size_t ip = 0; ip < program.size(); ++ip) {
        const Op& op = program[ip];
        emit(output, indent) << "// -- " << op.id.name << " --\n";
        output << labelName(static_cast<std::int64_t>(ip)) << ":\n";
        if (op.id == OpIds::Push) {
            emit(output, indent) << "_porth_stack.push(" << op.operand << ");\n";
        } else if (op.id == OpIds::Plus) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a + b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Minus) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a - b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Eq) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a == b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Ne) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a != b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Gt) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a > b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Lt) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a < b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Ge) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a >= b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Le) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a <= b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::If) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "if (a == 0) {\n";
            ++indent;
            emit(output, indent) << "goto " << labelName(op.operand) << ";\n";
            --indent;
            emit(output, indent) << "}\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Else) {
            emit(output, indent) << "goto " << labelName(op.operand) << ";\n";
        } else if (op.id == OpIds::End) {
            if (op.operand != static_cast<std::int64_t>(ip) + 1) {
                emit(output, indent) << "goto " << labelName(op.operand) << ";\n";
            }
        } else if (op.id == OpIds::Dump) {
            emit(output, indent) << "std::cout << _porth_stack.top() << \"\\n\";\n";
        } else if (op.id == OpIds::Dup) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.push(a);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Dup2) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a);\n";
            emit(output, indent) << "_porth_stack.push(b);\n";
            emit(output, indent) << "_porth_stack.push(a);\n";
            emit(output, indent) << "_porth_stack.push(b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Swap) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(b);\n";
            emit(output, indent) << "_porth_stack.push(a);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Drop) {
            emit(output, indent) << "_porth_stack.pop();\n";
        } else if (op.id == OpIds::While) {
            // nothing. just an anchor for the condition.
        } else if (op.id == OpIds::Do) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "if (a == 0) {\n";
            ++indent;
            emit(output, indent) << "goto " << labelName(op.operand) << ";\n";
            --indent;
            emit(output, indent) << "}\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Mem) {
            emit(output, indent) << "_porth_stack.push(0);\n";
        } else if (op.id == OpIds::Load) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto addr = static_cast<std::size_t>(a);\n";
            emit(output, indent) << "auto b = mem.at(addr);\n";
            emit(output, indent) << "_porth_stack.push(static_cast<std::int64_t>(b));\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Store) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto addr = static_cast<std::size_t>(a);\n";
            emit(output, indent) << "mem.at(addr) = static_cast<std::uint8_t>(b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Syscall1) {
            std::cerr << "not implemented: syscall1\n";
            return 1;
        } else if (op.id == OpIds::Syscall2) {
            std::cerr << "not implemented: syscall2\n";
            return 1;
        } else if (op.id == OpIds::Syscall3) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto syscallNumber = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto arg1 = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto arg2 = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto arg3 = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "if (syscallNumber == 1) {\n";
            ++indent;
            emit(output, indent) << "auto fd = arg1;\n";
            emit(output, indent) << "auto buf = arg2;\n";
            emit(output, indent) << "auto count = arg3;\n";
            emit(output, indent) << "const std::string_view s = {\n";
            ++indent;
            emit(output, indent) << "reinterpret_cast<const char*>(&mem[buf]),\n";
            emit(output, indent) << "static_cast<std::size_t>(count),\n";
            --indent;
            emit(output, indent) << "};\n";
            emit(output, indent) << "if (fd == 1) {\n";
            ++indent;
            emit(output, indent) << "std::cout << s;\n";
            --indent;
            emit(output, indent) << "} else if (fd == 2) {\n";
            ++indent;
            emit(output, indent) << "std::cerr << s;\n";
            --indent;
            emit(output, indent) << "} else {\n";
            ++indent;
            emit(output, indent) << "std::cerr << \"syscall3: unknown file descriptor \" << fd;\n";
            emit(output, indent) << "return 1;\n";
            --indent;
            emit(output, indent) << "}\n";
            --indent;
            emit(output, indent) << "} else {\n";
            ++indent;
            emit(output, indent) << "std::cerr << \"syscall3: unknown syscall \" << syscallNumber;\n";
            emit(output, indent) << "return 1;\n";
            --indent;
            emit(output, indent) << "}\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Syscall4) {
            std::cerr << "not implemented: syscall4\n";
            return 1;
        } else if (op.id == OpIds::Syscall5) {
            std::cerr << "not implemented: syscall5\n";
            return 1;
        } else if (op.id == OpIds::Syscall6) {
            std::cerr << "not implemented: syscall6\n";
            return 1;
        } else if (op.id == OpIds::Shr) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a >> b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Shl) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a << b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Bor) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a | b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Band) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a & b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Over) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a);\n";
            emit(output, indent) << "_porth_stack.push(b);\n";
            emit(output, indent) << "_porth_stack.push(a);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == OpIds::Mod) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a % b);\n";
            --indent;
            emit(output, indent) << "}\n";
        }
    }
    output << labelName(static_cast<std::int64_t>(program.size())) << ":\n";
    emit(output, indent) << "return 0;\n";
    --indent;
    emit(output, indent) << "}\n";
    return 0;
}
