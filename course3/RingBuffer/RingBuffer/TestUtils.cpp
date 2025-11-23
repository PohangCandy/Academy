#include "TestUtils.h"

std::random_device rd;
std::mt19937 gen(rd());

//----------------------------------------------------------------------
// minInclusive <= 난수 <= maxInclusive인 난수 생성 후 반환
//----------------------------------------------------------------------
int GetRandomNumber(int minInclusive, int maxInclusive)
{
    std::uniform_int_distribution<> dist(minInclusive, maxInclusive);
    return dist(gen);
}

//-----------------------------------------------------------------------------
// 랜덤 문자열을 반환하는 함수
//-----------------------------------------------------------------------------
std::string GetRandomString(int length)
{
    auto randchar = []() -> char
        {
            const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            constexpr size_t max = (sizeof(charset) - 1);
            return charset[GetRandomNumber(0, max)];
        };

    std::string tmp(length, 0);
    //tmp.begin() 부터 length만큼 randchar 반환값으로 채워넣기
    std::generate_n(tmp.begin(), length, randchar);

    return tmp;
}