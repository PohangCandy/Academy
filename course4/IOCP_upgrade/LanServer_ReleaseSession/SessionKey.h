#pragma once
#include "stdafx.h"

struct SessionKey
{
    inline static constexpr uint32_t INDEX_BITS = 20;
    inline static constexpr uint32_t ID_BITS = 44;

    inline static constexpr uint64_t ID_MASK = (1ULL << ID_BITS) - 1;

    uint64_t value;

    uint32_t GetIndex() const
    {
        return static_cast<uint32_t>(value >> ID_BITS);
    }

    uint64_t GetSessionId() const
    {
        return value & ID_MASK;
    }

    static SessionKey Make(uint32_t index, uint64_t sessionId)
    {
        return SessionKey{
            (static_cast<uint64_t>(index) << ID_BITS) |
            (sessionId & ID_MASK)
        };
    }
};