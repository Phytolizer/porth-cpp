#pragma once

#include <subprocess.h>

namespace subprocess {
struct DestroyGuard {
    subprocess_s* pProcess;
    explicit constexpr DestroyGuard(subprocess_s* pProcess) : pProcess(pProcess) {
    }
    ~DestroyGuard() {
        if (pProcess) {
            subprocess_destroy(pProcess);
        }
    }
    DestroyGuard(const DestroyGuard&) = delete;
    DestroyGuard(DestroyGuard&& other) : pProcess(other.pProcess) {
        other.pProcess = nullptr;
    }
    DestroyGuard& operator=(const DestroyGuard&) = delete;
    DestroyGuard& operator=(DestroyGuard&& other) {
        if (&other != this) {
            pProcess = other.pProcess;
            other.pProcess = nullptr;
        }
        return *this;
    }
};
} // namespace subprocess
