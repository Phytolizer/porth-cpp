#include "porth/op.hpp"

#include "iota_generated/op_id.hpp"

porth::Op::Op(const OpId id) : id(id), operand(0) {
}

porth::Op::Op(const OpId id, const std::int64_t operand) : id(id), operand(operand) {
}

porth::Op porth::push(const int x) {
    return Op{OpIds::Push, x};
}

porth::Op porth::plus() {
    return Op{OpIds::Plus};
}

porth::Op porth::minus() {
    return Op{OpIds::Minus};
}

porth::Op porth::equal() {
    return Op{OpIds::Equal};
}

porth::Op porth::gt() {
    return Op{OpIds::Gt};
}

porth::Op porth::lt() {
    return Op{OpIds::Lt};
}

porth::Op porth::iff() {
    return Op{OpIds::If};
}

porth::Op porth::elze() {
    return Op{OpIds::Else};
}

porth::Op porth::end() {
    return Op{OpIds::End};
}

porth::Op porth::dump() {
    return Op{OpIds::Dump};
}

porth::Op porth::dup() {
    return Op{OpIds::Dup};
}

porth::Op porth::dup2() {
    return Op{OpIds::Dup2};
}

porth::Op porth::swap() {
    return Op{OpIds::Swap};
}

porth::Op porth::wile() {
    return Op{OpIds::While};
}

porth::Op porth::doo() {
    return Op{OpIds::Do};
}

porth::Op porth::mem() {
    return Op{OpIds::Mem};
}

porth::Op porth::load() {
    return Op{OpIds::Load};
}

porth::Op porth::store() {
    return Op{OpIds::Store};
}

porth::Op porth::syscall1() {
    return Op{OpIds::Syscall1};
}

porth::Op porth::syscall2() {
    return Op{OpIds::Syscall2};
}

porth::Op porth::syscall3() {
    return Op{OpIds::Syscall3};
}

porth::Op porth::syscall4() {
    return Op{OpIds::Syscall4};
}

porth::Op porth::syscall5() {
    return Op{OpIds::Syscall5};
}

porth::Op porth::syscall6() {
    return Op{OpIds::Syscall6};
}
