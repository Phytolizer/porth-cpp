#include "porth/com.hpp"
#include "porth/lexer.hpp"
#include "porth/mem.hpp"
#include "porth/op.hpp"
#include "porth/parse_error.hpp"
#include "porth/semantic_error.hpp"
#include "porth/sim.hpp"
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

#ifdef _WIN32
#define EXE_SUFFIX ".exe"
#else
#define EXE_SUFFIX ""
#endif

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
        std::cerr << "[ERROR] " << args[0] << " invocation failed\n";
        return ret;
    }
    subprocess::DestroyGuard dg{&sub};
    char buffer[1024] = {};
    size_t nread;
    while ((nread = subprocess_read_stdout(&sub, buffer, sizeof buffer)) > 0) {
        std::cerr.write(buffer, static_cast<std::streamsize>(nread));
    }
    int code = 0;
    if (const int ret = subprocess_join(&sub, &code); ret != 0) {
        std::cerr << "[ERROR] failed to wait on " << args[0] << " process\n";
        return ret;
    }
    if (code != 0) {
        std::cerr << "[ERROR] " << args[0] << " returned non-zero exit code\n";
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
        "-static",
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
    std::cerr << "Usage: " << thisProgram << " [OPTIONS] <SUBCOMMAND> [ARGS]\n";
    std::cerr << "  OPTIONS:\n";
    std::cerr << "    -debug                 Enable debug mode\n";
    std::cerr << "  SUBCOMMANDS:\n";
    std::cerr << "    sim <file>             Simulate the program\n";
    std::cerr << "    com [OPTIONS] <file>   Compile the program\n";
}

porth::Op parseTokenAsOp(const porth::Token& token) {
    const auto& [filePath, row, col, word] = token;
    static_assert(porth::OpIds::Count.discriminant == 34, "Exhaustive handling of OpIds in parseTokenAsOp");
    if (word == "+") {
        return porth::plus();
    }
    if (word == "-") {
        return porth::minus();
    }
    if (word == "=") {
        return porth::eq();
    }
    if (word == "!=") {
        return porth::ne();
    }
    if (word == ">") {
        return porth::gt();
    }
    if (word == "<") {
        return porth::lt();
    }
    if (word == ">=") {
        return porth::ge();
    }
    if (word == "<=") {
        return porth::le();
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
    if (word == "dup2") {
        return porth::dup2();
    }
    if (word == "swap") {
        return porth::swap();
    }
    if (word == "drop") {
        return porth::drop();
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
    if (word == "syscall2") {
        return porth::syscall2();
    }
    if (word == "syscall3") {
        return porth::syscall3();
    }
    if (word == "syscall4") {
        return porth::syscall4();
    }
    if (word == "syscall5") {
        return porth::syscall5();
    }
    if (word == "syscall6") {
        return porth::syscall6();
    }
    if (word == "shr") {
        return porth::shr();
    }
    if (word == "shl") {
        return porth::shl();
    }
    if (word == "bor") {
        return porth::bor();
    }
    if (word == "band") {
        return porth::band();
    }
    if (word == "over") {
        return porth::over();
    }
    if (word == "mod") {
        return porth::mod();
    }
    std::int64_t pushArg;
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
    static_assert(porth::OpIds::Count.discriminant == 34, "Exhaustive handling of OpIds in crossReferenceBlocks");
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
                program[ip].operand = static_cast<std::int64_t>(ip) + 1;
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
        std::cerr << "[ERROR] no subcommand is provided\n";
        return 1;
    }

    bool debugMode = false;

    while (args.size() > cursor) {
        if (args[cursor] == "-debug"sv) {
            ++cursor;
            debugMode = true;
        } else {
            break;
        }
    }

    if (debugMode) {
        std::cout << "[INFO] Debug mode is enabled\n";
    }

    if (const char* const subcommand = args[cursor++]; subcommand == "sim"sv) {
        if (args.size() == cursor) {
            usage(thisProgram);
            std::cerr << "[ERROR] no input file is provided for the simulation\n";
            return 1;
        }
        const char* inputFilePath = args[cursor++];
        std::vector<porth::Op> program;
        try {
            program = loadProgramFromFile(inputFilePath);
        } catch (porth::ParseError& e) {
            std::cerr << "[ERROR] parse: " << e.what() << "\n";
            return 1;
        } catch (porth::SemanticError& e) {
            std::cerr << "[ERROR] semantic: " << e.what() << "\n";
            return 1;
        }

        try {
            porth::simulateProgram(program, debugMode);
        } catch (porth::SimulationError& e) {
            std::cerr << "[ERROR] " << e.what() << "\n";
            return 1;
        }
    } else if (subcommand == "com"sv) {
        if (args.size() == cursor) {
            usage(thisProgram);
            std::cerr << "[ERROR] no input file is provided for the simulation\n";
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
                        std::cerr << "[ERROR] no argument is provided for '-o'\n";
                        return 1;
                    }
                    outputFilePath = args[cursor++];
                } else {
                    std::cerr << "[ERROR] unknown flag '" << inputFilePathOrFlag << "'\n";
                    return 1;
                }
                if (args.size() == cursor) {
                    std::cerr << "[ERROR] no input file is provided for the simulation\n";
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
            std::cerr << "[ERROR] parse: " << e.what() << "\n";
            return 1;
        } catch (porth::SemanticError& e) {
            std::cerr << "[ERROR] semantic: " << e.what() << "\n";
            return 1;
        }

        const std::string cppOutputFilePath = std::string{PROJECT_BINARY_DIR} + "/output.cpp";
        if (const int ret = porth::compileProgram(program, cppOutputFilePath); ret != 0) {
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
        std::cerr << "[ERROR] unknown subcommand " << args[1] << "\n";
        return 1;
    }
    return 0;
}
