#include "porth/lexer.hpp"
#include "porth/op.hpp"
#include "porth/parse_error.hpp"
#include "porth/semantic_error.hpp"
#include "porth/simulation_error.hpp"

#include <cassert>
#include <config.hpp>
#include <fstream>
#include <iostream>
#include <iota_generated/op_id.hpp>
#include <regex>
#include <span/span.hpp>
#include <sstream>
#include <stack>
#include <string_view>
#include <subprocess.h>
#include <subprocess/destroy_guard.hpp>
#include <vector>

using namespace std::string_view_literals;

constexpr size_t MEM_CAPACITY = 640'000;
#ifdef _WIN32
#define EXE_SUFFIX ".exe"
#else
#define EXE_SUFFIX ""
#endif

template <typename T> T vecPop(std::vector<T>& v) {
    assert(!v.empty() && "vecPop: empty vector");
    T result = std::move(v.back());
    v.pop_back();
    return result;
}

void simulateProgram(const std::vector<porth::Op>& program) {
    static_assert(porth::OpIds::Count.discriminant == 17, "Exhaustive handling of OpIds in simulateProgram");
    std::vector<std::int64_t> stack;
    std::array<std::uint8_t, MEM_CAPACITY> mem;
    // execution is not linear, so we use a for loop with an index
    for (size_t ip = 0; ip < program.size();) {
        if (const porth::Op& op = program[ip]; op.id == porth::OpIds::Push) {
            stack.push_back(op.operand);
            ++ip;
        } else if (op.id == porth::OpIds::Plus) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a + b);
            ++ip;
        } else if (op.id == porth::OpIds::Minus) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a - b);
            ++ip;
        } else if (op.id == porth::OpIds::Equal) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a == b ? 1 : 0);
            ++ip;
        } else if (op.id == porth::OpIds::Gt) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a > b ? 1 : 0);
            ++ip;
        } else if (op.id == porth::OpIds::If) {
            if (const std::int64_t a = vecPop(stack); a == 0) {
                // simulate a goto
                ip = static_cast<size_t>(op.operand);
            } else {
                ++ip;
            }
        } else if (op.id == porth::OpIds::Else) {
            ip = static_cast<size_t>(op.operand);
        } else if (op.id == porth::OpIds::End) {
            ip = static_cast<size_t>(op.operand);
        } else if (op.id == porth::OpIds::Dump) {
            std::cout << stack.back() << "\n";
            ++ip;
        } else if (op.id == porth::OpIds::Dup) {
            const std::int64_t a = stack.back();
            stack.push_back(a);
            ++ip;
        } else if (op.id == porth::OpIds::While) {
            ++ip;
        } else if (op.id == porth::OpIds::Do) {
            const std::int64_t a = vecPop(stack);
            if (a == 0) {
                ip = static_cast<size_t>(op.operand);
            } else {
                ++ip;
            }
        } else if (op.id == porth::OpIds::Mem) {
            stack.push_back(0);
            ++ip;
        } else if (op.id == porth::OpIds::Load) {
            const std::int64_t a = vecPop(stack);
            // Interpret a as a memory address.
            // Here be dragons.
            const std::uintptr_t addr = static_cast<std::size_t>(a);
            if (addr >= mem.size()) {
                std::ostringstream errorMessage;
                errorMessage << "load: invalid memory address " << addr;
                throw porth::SimulationError(errorMessage.str());
            }
            const std::uint8_t b = mem[addr];
            stack.push_back(static_cast<std::int64_t>(b));
            ++ip;
        } else if (op.id == porth::OpIds::Store) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            // Interpret a as a memory address.
            // Here be dragons.
            const std::uintptr_t addr = static_cast<std::size_t>(a);
            if (addr >= mem.size()) {
                std::ostringstream errorMessage;
                errorMessage << "store: invalid memory address " << addr;
                throw porth::SimulationError(errorMessage.str());
            }
            mem[addr] = static_cast<std::uint8_t>(b);
            ++ip;
        } else if (op.id == porth::OpIds::Syscall1) {
            throw porth::SimulationError("syscall1: unimplemented");
        } else if (op.id == porth::OpIds::Syscall3) {
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
                    throw porth::SimulationError(errorMessage.str());
                }
            } else {
                std::ostringstream errorMessage;
                errorMessage << "syscall3: unknown syscall " << syscallNumber;
                throw porth::SimulationError(errorMessage.str());
            }
            ++ip;
        }
    }
}

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

int compileProgram(const std::vector<porth::Op>& program, const std::string& outFilePath) {
    std::ofstream output{outFilePath};
    if (!output) {
        std::cerr << "ERROR: failed to open '" << outFilePath << "' for writing\n";
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
    emit(output, indent) << "std::stack<int> _porth_stack;\n";
    static_assert(porth::OpIds::Count.discriminant == 17, "Exhaustive handling of OpIds in compileProgram");
    for (size_t ip = 0; ip < program.size(); ++ip) {
        const porth::Op& op = program[ip];
        emit(output, indent) << "// -- " << op.id.name << " --\n";
        output << labelName(ip) << ":\n";
        if (op.id == porth::OpIds::Push) {
            emit(output, indent) << "_porth_stack.push(" << op.operand << ");\n";
        } else if (op.id == porth::OpIds::Plus) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a + b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == porth::OpIds::Minus) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a - b);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == porth::OpIds::Equal) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a == b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == porth::OpIds::Gt) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto b = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "_porth_stack.push(a > b ? 1 : 0);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == porth::OpIds::If) {
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
        } else if (op.id == porth::OpIds::Else) {
            emit(output, indent) << "goto " << labelName(op.operand) << ";\n";
        } else if (op.id == porth::OpIds::End) {
            if (op.operand != static_cast<std::int64_t>(ip) + 1) {
                emit(output, indent) << "goto " << labelName(op.operand) << ";\n";
            }
        } else if (op.id == porth::OpIds::Dump) {
            emit(output, indent) << "std::cout << _porth_stack.top() << \"\\n\";\n";
        } else if (op.id == porth::OpIds::Dup) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.push(a);\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == porth::OpIds::While) {
            // nothing. just an anchor for the condition.
        } else if (op.id == porth::OpIds::Do) {
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
        } else if (op.id == porth::OpIds::Mem) {
            emit(output, indent) << "_porth_stack.push(0);\n";
        } else if (op.id == porth::OpIds::Load) {
            emit(output, indent) << "{\n";
            ++indent;
            emit(output, indent) << "auto a = _porth_stack.top();\n";
            emit(output, indent) << "_porth_stack.pop();\n";
            emit(output, indent) << "auto addr = static_cast<std::size_t>(a);\n";
            emit(output, indent) << "auto b = mem.at(addr);\n";
            emit(output, indent) << "_porth_stack.push(static_cast<std::int64_t>(b));\n";
            --indent;
            emit(output, indent) << "}\n";
        } else if (op.id == porth::OpIds::Store) {
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
        } else if (op.id == porth::OpIds::Syscall1) {
            std::cerr << "not implemented: syscall1\n";
            return 1;
        } else if (op.id == porth::OpIds::Syscall3) {
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
        }
    }
    output << labelName(static_cast<std::int64_t>(program.size())) << ":\n";
    emit(output, indent) << "return 0;\n";
    --indent;
    emit(output, indent) << "}\n";
    return 0;
}

void printArgs(const char* const* args) {
    for (const char* const* p = args; *p != nullptr; ++p) {
        std::cout << *p << " ";
    }
    std::cout << "\n";
}

int tryRunSubprocess(const char* const* args) {
    printArgs(args);
    subprocess_s sub{};
    if (const int ret = subprocess_create(
            args,
            subprocess_option_inherit_environment | subprocess_option_no_window |
                subprocess_option_combined_stdout_stderr | subprocess_option_enable_async,
            &sub);
        ret != 0) {
        std::cerr << "ERROR: " << args[0] << " invocation failed\n";
        return ret;
    }
    subprocess::DestroyGuard dg{&sub};
    char buffer[1024] = {};
    size_t nread = 0;
    while ((nread = subprocess_read_stdout(&sub, buffer, sizeof buffer)) > 0) {
        std::cerr.write(buffer, nread);
    }
    int code = 0;
    if (const int ret = subprocess_join(&sub, &code); ret != 0) {
        std::cerr << "ERROR: failed to wait on " << args[0] << " process\n";
        return ret;
    }
    if (code != 0) {
        std::cerr << "ERROR: " << args[0] << " returned non-zero exit code\n";
        return code;
    }
    return 0;
}

std::string replaceOrAppendExtension(
    const std::string& outFilePath, const std::regex& extensionPattern, const std::string& newExtension) {
    if (std::regex_search(outFilePath, extensionPattern)) {
        return std::regex_replace(outFilePath, extensionPattern, newExtension);
    }
    return outFilePath + newExtension;
}

int tryBuild(const std::string& cppOutputFilePath, const std::string& outFilePath) {
    constexpr const char* COMPILER = "clang++";

    const std::regex extensionPattern{"\\.cpp$"};

#ifdef _WIN32
    const std::string objPath = replaceOrAppendExtension(cppOutputFilePath, extensionPattern, ".obj");
#else
    const std::string objPath = replaceOrAppendExtension(cppOutputFilePath, extensionPattern, ".o");
#endif

#ifdef _MSC_VER
    const std::string outArg = "-Fo" + objPath;
    const std::string asmArg = "-Fa" + asmPath;
    const std::string exeArg = "-Fe" + outFilePath;
    const char* clArgs[] = {
        "cl",
        "-nologo",           // suppress copyright message
        "-w",                // suppress warning output
        "-TP",               // this is c++ code
        "-std:c++20",        // set c++ standard
        "-O2",               // optimization level
        "-EHsc",             // c++ exception option
        outFilePath.c_str(), // file to compile
        outArg.c_str(),      // obj name
        asmArg.c_str(),      // also generate assembly
        exeArg.c_str(),      // also generate executable
        "-link",             // i want to link as well
        nullptr,
    };
    if (const int ret = tryRunSubprocess(clArgs); ret != 0) {
        return ret;
    }
#else
    const char* objArgs[] = {
        "/usr/bin/env",
        COMPILER,
        "-w",
        "-xc++",
        "-std=c++20",
        "-O2",
        "-march=native",
        "-c",
        cppOutputFilePath.c_str(),
        "-o",
        objPath.c_str(),
        nullptr,
    };
    if (const int ret = tryRunSubprocess(objArgs); ret != 0) {
        return ret;
    }
    const char* linkArgs[] = {
        "/usr/bin/env",
        COMPILER,
        "-w",
        "-flto",
        "-march=native",
        objPath.c_str(),
        "-o",
        outFilePath.c_str(),
        nullptr,
    };
    if (const int ret = tryRunSubprocess(linkArgs); ret != 0) {
        return ret;
    }
#endif

    return 0;
}

int tryRunExecutable(const std::string& outFilePath) {
    const char* exeArgs[] = {
        outFilePath.c_str(),
        nullptr,
    };
    return tryRunSubprocess(exeArgs);
}

void usage(const char* thisProgram) {
    std::cerr << "Usage: " << thisProgram << " <SUBCOMMAND> [ARGS]\n";
    std::cerr << "SUBCOMMANDS:\n";
    std::cerr << "    sim <file>   Simulate the program\n";
    std::cerr << "    com <file>   Compile the program\n";
}

porth::Op parseTokenAsOp(const porth::Token& token) {
    const auto& [filePath, row, col, word] = token;
    static_assert(porth::OpIds::Count.discriminant == 17, "Exhaustive handling of OpIds in parseTokenAsOp");
    if (word == "+") {
        return porth::plus();
    }
    if (word == "-") {
        return porth::minus();
    }
    if (word == "=") {
        return porth::equal();
    }
    if (word == ">") {
        return porth::gt();
    }
    if (word == "dump") {
        return porth::dump();
    }
    if (word == "if") {
        return porth::iff();
    }
    if (word == "else") {
        return porth::elze();
    }
    if (word == "end") {
        return porth::end();
    }
    if (word == "dup") {
        return porth::dup();
    }
    if (word == "while") {
        return porth::wile();
    }
    if (word == "do") {
        return porth::doo();
    }
    if (word == "mem") {
        return porth::mem();
    }
    if (word == ",") {
        return porth::load();
    }
    if (word == ".") {
        return porth::store();
    }
    if (word == "syscall1") {
        return porth::syscall1();
    }
    if (word == "syscall3") {
        return porth::syscall3();
    }
    int pushArg;
    if (std::istringstream wordStream{word}; !(wordStream >> pushArg)) {
        std::cerr << filePath << ":" << row << ":" << col << ": attempt to convert non-integer value\n";
        throw std::runtime_error{"attempt to convert non-integer value"};
    }
    return porth::push(pushArg);
}

template <typename T> T stackPop(std::stack<T>& stack) {
    T value = std::move(stack.top());
    stack.pop();
    return value;
}

std::vector<porth::Op> crossReferenceBlocks(std::vector<porth::Op>&& program) {
    std::stack<size_t> stack;
    static_assert(porth::OpIds::Count.discriminant == 17, "Exhaustive handling of OpIds in crossReferenceBlocks");
    for (size_t ip = 0; ip < program.size(); ++ip) {
        if (const porth::Op& op = program[ip]; op.id == porth::OpIds::If) {
            stack.push(ip);
        } else if (op.id == porth::OpIds::Else) {
            const size_t ifIp = stackPop(stack);
            program[ifIp].operand = static_cast<std::int64_t>(ip) + 1;
            stack.push(ip);
        } else if (op.id == porth::OpIds::End) {
            if (const size_t blockIp = stackPop(stack);
                program[blockIp].id == porth::OpIds::If || program[blockIp].id == porth::OpIds::Else) {
                program[blockIp].operand = static_cast<std::int64_t>(ip);
                program[ip].operand = ip + 1;
            } else if (program[blockIp].id == porth::OpIds::Do) {
                program[ip].operand = program[blockIp].operand;
                program[blockIp].operand = static_cast<std::int64_t>(ip) + 1;
            } else {
                throw porth::SemanticError{"`end` can only close if and while blocks for now"};
            }
        } else if (op.id == porth::OpIds::While) {
            stack.push(ip);
        } else if (op.id == porth::OpIds::Do) {
            const size_t whileIp = stackPop(stack);
            program[ip].operand = static_cast<std::int64_t>(whileIp);
            stack.push(ip);
        }
    }
    return program;
}

std::vector<porth::Op> loadProgramFromFile(const std::string& inputFilePath) {
    std::vector<porth::Op> result;
    for (const porth::Token& token : porth::lexFile(inputFilePath)) {
        result.emplace_back(parseTokenAsOp(token));
    }
    return crossReferenceBlocks(std::move(result));
}

int main(const int argc, char** argv) {
    const span::Span<char*> args{argv, static_cast<size_t>(argc)};
    size_t cursor = 0;
    const char* const thisProgram = args[cursor++];

    if (args.size() == cursor) {
        usage(thisProgram);
        std::cerr << "ERROR: no subcommand is provided\n";
        return 1;
    }

    if (const char* const subcommand = args[cursor++]; subcommand == "sim"sv) {
        if (args.size() == cursor) {
            usage(thisProgram);
            std::cerr << "ERROR: no input file is provided for the simulation\n";
            return 1;
        }
        const char* inputFilePath = args[cursor++];
        std::vector<porth::Op> program;
        try {
            program = loadProgramFromFile(inputFilePath);
        } catch (porth::ParseError& e) {
            std::cerr << "ERROR: parse: " << e.what() << "\n";
            return 1;
        } catch (porth::SemanticError& e) {
            std::cerr << "ERROR: semantic: " << e.what() << "\n";
            return 1;
        }

        try {
            simulateProgram(program);
        } catch (porth::SimulationError& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            return 1;
        }
    } else if (subcommand == "com"sv) {
        if (args.size() == cursor) {
            usage(thisProgram);
            std::cerr << "ERROR: no input file is provided for the simulation\n";
            return 1;
        }
        const char* inputFilePathOrFlag = args[cursor++];
        bool runExecutable = false;
        std::string inputFilePath;
        std::string outputFilePath = std::string{PROJECT_BINARY_DIR} + "/output" EXE_SUFFIX;
        if (inputFilePathOrFlag[0] == '-') {
            while (inputFilePathOrFlag[0] == '-') {
                const char* const flag = inputFilePathOrFlag + 1;
                if (flag == "r"sv) {
                    runExecutable = true;
                } else if (flag == "o"sv) {
                    if (args.size() == cursor) {
                        std::cerr << "ERROR: no argument is provided for '-o'\n";
                        return 1;
                    }
                    outputFilePath = args[cursor++];
                } else {
                    std::cerr << "ERROR: unknown flag '" << inputFilePathOrFlag << "'\n";
                    return 1;
                }
                if (args.size() == cursor) {
                    std::cerr << "ERROR: no input file is provided for the simulation\n";
                    return 1;
                }
                inputFilePathOrFlag = args[cursor++];
            }
            inputFilePath = inputFilePathOrFlag;
        } else {
            inputFilePath = inputFilePathOrFlag;
        }
        std::vector<porth::Op> program;
        try {
            program = loadProgramFromFile(inputFilePath);
        } catch (porth::ParseError& e) {
            std::cerr << "ERROR: parse: " << e.what() << "\n";
            return 1;
        } catch (porth::SemanticError& e) {
            std::cerr << "ERROR: semantic: " << e.what() << "\n";
            return 1;
        }

        const std::string cppOutputFilePath = std::string{PROJECT_BINARY_DIR} + "/output.cpp";
        if (const int ret = compileProgram(program, cppOutputFilePath); ret != 0) {
            return ret;
        }
        if (const int ret = tryBuild(cppOutputFilePath, outputFilePath); ret != 0) {
            return ret;
        }
        if (runExecutable) {
            if (const int ret = tryRunExecutable(outputFilePath); ret != 0) {
                return ret;
            }
        }
    } else {
        usage(thisProgram);
        std::cerr << "ERROR: unknown subcommand " << args[1] << "\n";
        return 1;
    }
    return 0;
}
