#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include <ctime>
#include <cstring>

// =======================================================
// CPacket.h / CPacket.cpp 는 네가 올린 코드 그대로 포함
// =======================================================

#include "CPacket.h"
#include "CPacketRingBuffer.h"

// =======================================================
// 랜덤 패킷 생성
// =======================================================

CPacket* MakeRandomPacket(std::mt19937& rng)
{
    std::uniform_int_distribution<int> intDist(-100000, 100000);
    std::uniform_real_distribution<float> floatDist(-1000.f, 1000.f);
    std::uniform_int_distribution<int> typeDist(0, 5);

    CPacket* pkt = new CPacket();

    int fieldCount = 1 + (rng() % 6);
    *pkt << fieldCount;

    for (int i = 0; i < fieldCount; ++i)
    {
        int type = typeDist(rng);
        *pkt << type;

        switch (type)
        {
        case 0: *pkt << intDist(rng); break;
        case 1: *pkt << floatDist(rng); break;
        case 2: *pkt << (__int64)intDist(rng); break;
        case 3: *pkt << (short)intDist(rng); break;
        case 4: *pkt << (unsigned char)(rng() % 255); break;
        case 5: *pkt << (double)floatDist(rng); break;
        }
    }

    return pkt;
}

// =======================================================
// 패킷 raw 비교
// =======================================================

bool IsPacketEqual(CPacket* a, CPacket* b)
{
    if (!a || !b) return false;
    if (a->GetDataSize() != b->GetDataSize()) return false;

    std::vector<char> bufA(a->GetDataSize());
    std::vector<char> bufB(b->GetDataSize());

    CPacket copyA = *a;
    CPacket copyB = *b;

    copyA.GetData(bufA.data(), (int)bufA.size());
    copyB.GetData(bufB.data(), (int)bufB.size());

    return std::memcmp(bufA.data(), bufB.data(), bufA.size()) == 0;
}


// =======================================================
// main - 랜덤 스트레스 테스트
// =======================================================

int main()
{
    std::mt19937 rng((unsigned)time(nullptr));

    CPacketRingBuffer ring(128);
    std::vector<CPacket*> referenceQueue;

    constexpr int TEST_COUNT = 200000;
    std::uniform_int_distribution<int> actionDist(0, 1);

    for (int i = 0; i < TEST_COUNT; ++i)
    {
        bool doEnqueue = referenceQueue.empty() || actionDist(rng) == 0;

        if (doEnqueue)
        {
            CPacket* pkt = MakeRandomPacket(rng);
            if (ring.Enqueue(pkt))
            {
                referenceQueue.push_back(pkt);
            }
            else
            {
                delete pkt;
            }
        }
        else
        {
            CPacket* out = nullptr;
            if (ring.Dequeue(out))
            {
                assert(!referenceQueue.empty());

                CPacket* expected = referenceQueue.front();
                referenceQueue.erase(referenceQueue.begin());

                assert(IsPacketEqual(out, expected));

                delete expected;
                delete out;
            }
        }
    }

    // 남은 패킷 정리
    while (!referenceQueue.empty())
    {
        CPacket* out = nullptr;
        bool ok = ring.Dequeue(out);
        assert(ok);

        CPacket* expected = referenceQueue.front();
        referenceQueue.erase(referenceQueue.begin());

        assert(IsPacketEqual(out, expected));

        delete expected;
        delete out;
    }

    std::cout << "Packet RingBuffer Test PASSED" << std::endl;
    return 0;
}
