#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <regex>
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

int main() {
    const std::filesystem::recursive_directory_iterator tests{std::filesystem::current_path() / "tests"};
    for (const auto& entry : tests) {
        if (entry.is_directory()) {
            continue;
        }
        if (entry.path().extension() != ".porth") {
            continue;
        }

        std::cout << "[INFO] Testing " << entry.path().filename().string() << "\n";
        try {
            const std::string simOutput = runSubprocess({PORTH_CPP_EXE, "sim", entry.path().string()});
            std::filesystem::path exePath = entry.path();
            exePath.replace_extension();
            runSubprocess({
                PORTH_CPP_EXE,
                "com",
                "-o",
                exePath,
                entry.path().string(),
            });
            const std::string comOutput = runSubprocess({exePath});
            if (simOutput != comOutput) {
                std::cout << "[ERROR] Output discrepancy between simulation and compilation\n";
                std::cout << "  Simulation output:\n";
                std::istringstream simStream{simOutput};
                std::string line;
                while (std::getline(simStream, line)) {
                    std::cout << "    " << line << "\n";
                }
                std::cout << "  Compilation output:\n";
                std::istringstream comStream{comOutput};
                while (std::getline(comStream, line)) {
                    std::cout << "    " << line << "\n";
                }
                return 1;
            } else {
                std::cout << "[INFO] " << entry.path().filename().string() << " OK\n";
            }
        } catch (const SubprocessError& e) {
            std::cout << "[ERROR] " << e.what() << "\n";
            return 1;
        }
    }
}
