#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <span/span.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <subprocess.h>
#include <subprocess/destroy_guard.hpp>
#include <testconfig.hpp>
#include <vector>

struct SubprocessError : std::runtime_error {
    SubprocessError(const std::string& message) : std::runtime_error{message} {
    }
};

std::string runSubprocess(const std::vector<std::string>& args) {
    std::cout << "[CMD] ";
    for (std::size_t i = 0; i < args.size(); ++i) {
        std::cout << args[i];
        if (i < args.size() - 1) {
            std::cout << " ";
        }
    }
    std::cout << "\n";

    subprocess_s sub;
    std::vector<const char*> cArgs;
    std::transform(
        args.begin(), args.end(), std::back_inserter(cArgs), [](const std::string& arg) { return arg.c_str(); });
    cArgs.push_back(nullptr);
    if (const int ret = subprocess_create(
            cArgs.data(),
            subprocess_option_enable_async | subprocess_option_combined_stdout_stderr |
                subprocess_option_inherit_environment | subprocess_option_no_window,
            &sub);
        ret != 0) {
        std::ostringstream errorMessage;
        errorMessage << "failed to spawn subprocess for '" << args[0] << "'";
        throw SubprocessError{errorMessage.str()};
    }

    char buffer[1024];
    std::string result;
    int nread;
    while ((nread = subprocess_read_stdout(&sub, buffer, sizeof buffer)) > 0) {
        result.append(buffer, nread);
    }
    int code = 0;
    if (const int ret = subprocess_join(&sub, &code); ret != 0) {
        std::ostringstream errorMessage;
        errorMessage << "failed to join subprocess for '" << args[0] << "'";
        throw SubprocessError{errorMessage.str()};
    }
    if (code != 0) {
        std::cerr << result << "\n";
        std::ostringstream errorMessage;
        errorMessage << "'" << args[0] << "' failed with code " << code;
        throw SubprocessError{errorMessage.str()};
    }

    return result;
}

int test() {
    const std::filesystem::recursive_directory_iterator tests{std::filesystem::current_path() / "tests"};
    for (const auto& entry : tests) {
        if (entry.is_directory()) {
            continue;
        }
        if (entry.path().extension() != ".porth") {
            continue;
        }

        std::cout << "[INFO] Testing " << entry.path().filename().string() << "\n";

        std::filesystem::path txtPath = entry.path();
        txtPath.replace_extension(".txt");
        std::ifstream txtFile{txtPath.string()};
        std::ostringstream txtContentsStream;
        if (!(txtContentsStream << txtFile.rdbuf())) {
            std::cerr << "[ERROR] failed to read " << txtPath.string() << "\n";
            return 1;
        }
        const std::string expectedOutput = txtContentsStream.str();

        try {
            const std::string simOutput = runSubprocess({PORTH_CPP_EXE, "sim", entry.path().string()});
            if (simOutput != expectedOutput) {
                std::cerr << "[ERROR] Unexpected simulation output\n";
                std::cerr << "  Expected:\n";
                std::istringstream expectedStream{expectedOutput};
                std::string line;
                while (std::getline(expectedStream, line)) {
                    std::cerr << "    " << line << "\n";
                }
                std::cerr << "  Simulation output:\n";
                std::istringstream simStream{simOutput};
                while (std::getline(simStream, line)) {
                    std::cerr << "    " << line << "\n";
                }
                return 1;
            }

            std::filesystem::path exePath = entry.path();
#ifdef _WIN32
            exePath.replace_extension(".exe");
#else
            exePath.replace_extension();
#endif
            runSubprocess({
                PORTH_CPP_EXE,
                "com",
                "-o",
                exePath,
                entry.path().string(),
            });
            const std::string comOutput = runSubprocess({exePath});
            if (comOutput != expectedOutput) {
                std::cerr << "[ERROR] Unexpected compilation output\n";
                std::cerr << "  Expected:\n";
                std::istringstream expectedStream{expectedOutput};
                std::string line;
                while (std::getline(expectedStream, line)) {
                    std::cerr << "    " << line << "\n";
                }
                std::cerr << "  Compilation output:\n";
                std::istringstream comStream{comOutput};
                while (std::getline(comStream, line)) {
                    std::cerr << "    " << line << "\n";
                }
                return 1;
            }

            std::cout << "[INFO] " << entry.path().filename().string() << " OK\n";
        } catch (const SubprocessError& e) {
            std::cout << "[ERROR] " << e.what() << "\n";
            return 1;
        }
    }
    return 0;
}

void record() {
    const std::filesystem::recursive_directory_iterator tests{std::filesystem::current_path() / "tests"};
    for (const auto& entry : tests) {
        if (entry.is_directory()) {
            continue;
        }
        if (entry.path().extension() != ".porth") {
            continue;
        }

        std::cout << "[INFO] Recording " << entry.path().filename().string() << "\n";

        std::filesystem::path txtPath = entry.path();
        txtPath.replace_extension(".txt");
        std::ostringstream txtContentsStream;
        try {
            const std::string simOutput = runSubprocess({PORTH_CPP_EXE, "sim", entry.path().string()});
            txtContentsStream << simOutput;
        } catch (const SubprocessError& e) {
            std::cout << "[ERROR] " << e.what() << "\n";
            return;
        }
        std::ofstream txtFile{txtPath.string()};
        if (!(txtFile << txtContentsStream.str())) {
            std::cerr << "[ERROR] failed to write " << txtPath.string() << "\n";
            return;
        }
    }
}

void usage(const std::string& exeName) {
    std::cout << "Usage: " << exeName << " [SUBCOMMAND]\n";
    std::cout << "  test      Run the tests. (Default when no subcommand is provided)\n";
    std::cout << "  record    Record expected output for the tests.\n";
    std::cout << "  help      Print this message to stdout.\n";
}

int main(int argc, char** argv) {
    span::Span<char*> args{argv, static_cast<std::size_t>(argc)};
    std::size_t cursor = 0;
    const std::string exeName = args[cursor++];

    if (args.size() == cursor) {
        return test();
    }

    const std::string subcmd = args[cursor++];
    if (subcmd == "record") {
        record();
        return 0;
    }
    if (subcmd == "test") {
        return test();
    }
    if (subcmd == "help") {
        usage(exeName);
        return 0;
    }
    usage(exeName);
    std::cerr << "[ERROR] Unknown subcommand '" << subcmd << "'\n";
    return 1;
}
