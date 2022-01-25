#pragma once

#include <cstdint>
#include <iota_generated/op_id.hpp>
#include <ostream>

namespace porth {

struct Op {
    OpId id;
    std::int64_t operand;

    explicit Op(OpId id);
    Op(OpId id, std::int64_t operand);
};

Op push(std::int64_t x);
Op plus();
Op minus();
Op eq();
Op ne();
Op gt();
Op lt();
Op ge();
Op le();
Op iff();
Op elze();
Op end();
Op print();
Op dup();
Op dup2();
Op swap();
Op drop();
Op wile();
Op doo();
Op mem();
Op load();
Op store();
Op syscall1();
Op syscall2();
Op syscall3();
Op syscall4();
Op syscall5();
Op syscall6();
Op shr();
Op shl();
Op bor();
Op band();
Op over();
Op mod();

} // namespace porth

std::ostream& operator<<(std::ostream& os, const porth::Op& op);
