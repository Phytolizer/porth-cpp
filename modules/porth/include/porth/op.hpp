#pragma once

#include <cstdint>
#include <iota_generated/op_id.hpp>

namespace porth {

struct Op {
    OpId id;
    std::int64_t operand;

    explicit Op(OpId id);
    Op(OpId id, std::int64_t operand);
};

Op push(int x);
Op plus();
Op minus();
Op equal();
Op gt();
Op iff();
Op elze();
Op end();
Op dump();
Op dup();

} // namespace porth
