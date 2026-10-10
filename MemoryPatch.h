#pragma once

#include <cstdint>
#include <cstddef>

bool WriteAbsoluteJump6(
    std::uintptr_t source,
    const void* destination);

// Same push-imm32/ret absolute detour as WriteAbsoluteJump6, but
// NOP-pads a whole-instruction patch span >= 6 bytes.
bool WriteAbsoluteJumpPadded(
    std::uintptr_t source,
    const void* destination,
    std::size_t patchLength);