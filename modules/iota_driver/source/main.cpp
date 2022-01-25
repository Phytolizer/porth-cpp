#include <fstream>
#include <iostream>
#include <iota/iota.hpp>
#include <span/span.hpp>

int main(const int argc, char** argv) {
    const span::Span<char*> args{argv, static_cast<size_t>(argc)};
    if (args.size() != 3) {
        std::cerr << "Usage: " << args[0] << " <inFile> <outFile>\n";
        return 1;
    }
    const std::ifstream inStream{args[1]};
    if (!inStream) {
        std::cerr << "ERROR: Could not open " << args[1] << ".\n";
        return 2;
    }
    std::ofstream outStream{args[2]};
    if (!outStream) {
        std::cerr << "ERROR: Could not open " << args[2] << " for writing.\n";
        return 3;
    }
    return iota::generate(inStream, outStream);
}
