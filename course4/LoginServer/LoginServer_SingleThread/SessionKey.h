#pragma once
#include "stdafx.h"
//#include "CommonProtocol.h"

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

    uint64_t GetSessionKey() const
    {
        return value;
    }

    static SessionKey MakeKey(uint32_t index, uint64_t sessionId)
    {
        if (index >= 50000)
        {
            printf("인덱스와 세션 아이디 폭증을 막을 방어코드 필요함\n");
            __debugbreak();
        }
        return SessionKey{
            (static_cast<uint64_t>(index) << ID_BITS) |
            (sessionId & ID_MASK)
        };
    }

    bool operator==(const SessionKey& other) const noexcept
    {
        return value == other.value;
    }

    bool operator!=(const SessionKey& other) const noexcept
    {
        return value != other.value;
    }
};