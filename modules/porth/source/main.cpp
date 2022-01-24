#include "porth/lexer.hpp"
#include "porth/op.hpp"

#include <config.hpp>
#include <fstream>
#include <iostream>
#include <iota_generated/op_id.hpp>
#include <regex>
#include <span>
#include <sstream>
#include <stack>
#include <string_view>
#include <subprocess/destroy_guard.hpp>
#include <subprocess/subprocess.h>
#include <vector>

using namespace std::string_view_literals;

template <typename T> T vecPop(std::vector<T>& v) {
    T result = std::move(v.back());
    v.pop_back();
    return result;
}

void simulateProgram(const std::vector<porth::Op>& program) {
    static_assert(porth::OpIds::Count.discriminant == 8, "Exhaustive handling of OpIds in simulateProgram");
    std::vector<std::int64_t> stack;
    // execution is not linear, so we use a for loop with an index
    for (size_t ip = 0; ip < program.size();) {
        if (const porth::Op& op = program[ip]; op.id == porth::OpIds::Push) {
            stack.push_back(op.operand);
        } else if (op.id == porth::OpIds::Plus) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a + b);
        } else if (op.id == porth::OpIds::Minus) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a - b);
        } else if (op.id == porth::OpIds::Equal) {
            const std::int64_t b = vecPop(stack);
            const std::int64_t a = vecPop(stack);
            stack.push_back(a == b ? 1 : 0);
        } else if (op.id == porth::OpIds::If) {
            if (const std::int64_t a = vecPop(stack); a == 0) {
                // simulate a goto
                ip = static_cast<size_t>(op.operand);
                continue;
            }
        } else if (op.id == porth::OpIds::Else) {
            ip = static_cast<size_t>(op.operand);
            continue;
        } else if (op.id == porth::OpIds::End) {
            // nothing. end is just a marker for crossReferenceBlocks, not an actual opcode
        } else if (op.id == porth::OpIds::Dump) {
            std::cout << stack.back() << "\n";
        }
        ++ip;
    }
}

std::string labelName(const std::int64_t offset) {
    std::ostringstream result;
    result << "_porth_addr_" << offset;
    return result.str();
}

int compileProgram(const std::vector<porth::Op>& program, const std::string& outFilePath) {
    std::ofstream output{outFilePath};
    if (!output) {
        std::cerr << "ERROR: failed to open '" << outFilePath << "' for writing\n";
        return 1;
    }
    output << "#include <iostream>\n";
    output << "#include <stack>\n";
    output << "int main() {\n";
    output << "    std::stack<int> s;\n";
    static_assert(porth::OpIds::Count.discriminant == 8, "Exhaustive handling of OpIds in compileProgram");
    for (size_t ip = 0; ip < program.size(); ++ip) {
        const porth::Op& op = program[ip];
        output << "    // -- " << op.id.name << " --\n";
        if (op.id == porth::OpIds::Push) {
            output << "    s.push(" << op.operand << ");\n";
        } else if (op.id == porth::OpIds::Plus) {
            output << "    {\n";
            output << "        auto b = s.top();\n";
            output << "        s.pop();\n";
            output << "        auto a = s.top();\n";
            output << "        s.pop();\n";
            output << "        s.push(a + b);\n";
            output << "    }\n";
        } else if (op.id == porth::OpIds::Minus) {
            output << "    {\n";
            output << "        auto b = s.top();\n";
            output << "        s.pop();\n";
            output << "        auto a = s.top();\n";
            output << "        s.pop();\n";
            output << "        s.push(a - b);\n";
            output << "    }\n";
        } else if (op.id == porth::OpIds::Equal) {
            output << "    {\n";
            output << "        auto b = s.top();\n";
            output << "        s.pop();\n";
            output << "        auto a = s.top();\n";
            output << "        s.pop();\n";
            output << "        s.push(a == b ? 1 : 0);\n";
            output << "    }\n";
        } else if (op.id == porth::OpIds::If) {
            output << "    {\n";
            output << "        auto a = s.top();\n";
            output << "        s.pop();\n";
            output << "        if (a == 0) {\n";
            output << "            goto " << labelName(op.operand) << ";\n";
            output << "        }\n";
            output << "    }\n";
        } else if (op.id == porth::OpIds::End) {
            output << labelName(static_cast<std::int64_t>(ip)) << ":\n";
        } else if (op.id == porth::OpIds::Dump) {
            output << "    std::cout << s.top() << \"\\n\";\n";
        }
    }
    output << "    return 0;\n";
    output << "}\n";
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
                subprocess_option_combined_stdout_stderr,
            &sub);
        ret != 0) {
        std::cerr << "ERROR: " << args[0] << " invocation failed\n";
        return ret;
    }
    subprocess::DestroyGuard dg{&sub};
    int code = 0;
    if (const int ret = subprocess_join(&sub, &code); ret != 0) {
        std::cerr << "ERROR: failed to wait on " << args[0] << " process\n";
        return ret;
    }
    char buffer[1024] = {};
    std::FILE* fp = subprocess_stdout(&sub);
    while (fgets(buffer, sizeof buffer, fp) != nullptr) {
        std::cout << buffer;
    }
    if (code != 0) {
        std::cerr << "ERROR: " << args[0] << " process reported failure with the above log\n";
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

int tryBuild(const std::string& outFilePath) {
    const std::regex extensionPattern{"\\.cpp$"};
    const std::string objPath = replaceOrAppendExtension(outFilePath, extensionPattern, ".obj");
    const std::string exePath = replaceOrAppendExtension(outFilePath, extensionPattern, ".exe");
    const std::string asmPath = replaceOrAppendExtension(outFilePath, extensionPattern, ".asm");

    const std::string outArg = "-Fo" + objPath;
    const std::string asmArg = "-Fa" + asmPath;
    const std::string exeArg = "-Fe" + exePath;
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

    return 0;
}

void usage(const char* thisProgram) {
    std::cerr << "Usage: " << thisProgram << " <SUBCOMMAND> [ARGS]\n";
    std::cerr << "SUBCOMMANDS:\n";
    std::cerr << "    sim <file>   Simulate the program\n";
    std::cerr << "    com <file>   Compile the program\n";
}

struct UnconsArgs {
    char* first;
    std::span<char*> rest;
    constexpr UnconsArgs(char* first, const std::span<char*> rest) : first(first), rest(rest) {
    }
};

UnconsArgs uncons(std::span<char*> args) {
    return {args[0], args.subspan(1)};
}

porth::Op parseTokenAsOp(const porth::Token& token) {
    const auto& [filePath, row, col, word] = token;
    static_assert(porth::OpIds::Count.discriminant == 8, "Exhaustive handling of OpIds in parseTokenAsOp");
    if (word == "+") {
        return porth::plus();
    }
    if (word == "-") {
        return porth::minus();
    }
    if (word == "=") {
        return porth::equal();
    }
    if (word == ".") {
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
    static_assert(porth::OpIds::Count.discriminant == 8, "Exhaustive handling of OpIds in crossReferenceBlocks");
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
            } else {
                throw std::runtime_error{"`end` can only close if/else blocks for now"};
            }
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
    const std::span args{argv, static_cast<size_t>(argc)};
    const auto [thisProgram, args2] = uncons(args);

    if (args2.empty()) {
        usage(thisProgram);
        std::cerr << "ERROR: no subcommand is provided\n";
        return 1;
    }

    if (const auto [subcommand, args3] = uncons(args2); subcommand == "sim"sv) {
        if (args3.empty()) {
            usage(thisProgram);
            std::cerr << "ERROR: no input file is provided for the simulation\n";
            return 1;
        }
        const auto [inputFilePath, args4] = uncons(args3);
        const std::vector<porth::Op> program = loadProgramFromFile(inputFilePath);
        simulateProgram(program);
    } else if (subcommand == "com"sv) {
        if (args3.empty()) {
            usage(thisProgram);
            std::cerr << "ERROR: no input file is provided for the simulation\n";
            return 1;
        }
        const auto [inputFilePath, args4] = uncons(args3);
        const std::vector<porth::Op> program = loadProgramFromFile(inputFilePath);
        const std::string outFilePath = std::string{PROJECT_BINARY_DIR} + "/output.cpp";
        if (const int ret = compileProgram(program, outFilePath); ret != 0) {
            return ret;
        }
        if (const int ret = tryBuild(outFilePath); ret != 0) {
            return ret;
        }
    } else {
        usage(thisProgram);
        std::cerr << "ERROR: unknown subcommand " << args[1] << "\n";
        return 1;
    }
    return 0;
}
