#pragma once

#include "subprocess.h"

namespace subprocess {

struct DestroyGuard {
    subprocess_s* p;
    explicit DestroyGuard(subprocess_s* p) : p(p) {
    }
    ~DestroyGuard() {
        subprocess_destroy(p);
    }
    DestroyGuard(const DestroyGuard&) = delete;
    DestroyGuard(DestroyGuard&&) = delete;
    DestroyGuard& operator=(const DestroyGuard&) = delete;
    DestroyGuard& operator=(DestroyGuard&&) = delete;
};

} // namespace subprocess
