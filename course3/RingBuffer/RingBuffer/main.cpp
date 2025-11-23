#include <iostream>
#include <list>

#include "TestUtils.h"
#include "CRingBuffer.h"

using namespace std;

enum class eTestAction
{
    Enqueue,
    Dequeue,
    Peek,
    Count
};

char buf[10000];

void Test(const int size)
{
    CRingBuffer rb(size);
    string referenceData;
    referenceData.reserve(size);

    int capacity = size;
    int useSize = 0;
    int freeSize = size;

    int testCount = GetRandomNumber(10000, 50000);
    for (int i = 0; i < testCount; ++i)
    {
        ExpectEqual(capacity, rb.GetBufferSize(), "Capacity");
        ExpectEqual(useSize, rb.GetUseSize(), "UseSize");
        ExpectEqual(freeSize, rb.GetFreeSize(), "FreeSize");
        switch (static_cast<eTestAction>(GetRandomNumber(0, static_cast<int>(eTestAction::Count) - 1)))
        {
        case eTestAction::Enqueue:
        {
            string enqueueData = GetRandomString(GetRandomNumber(1, size));
            int enqueueSize = static_cast<int>(enqueueData.length());
            int expectedEnqueueResult;
            if (freeSize >= enqueueSize)
            {
                expectedEnqueueResult = enqueueSize;
            }
            else
            {
                expectedEnqueueResult = 0;
            }
            ExpectEqual(rb.Enqueue(enqueueData.c_str(), enqueueData.length()), expectedEnqueueResult, "Enqueue Result");
            referenceData.append(enqueueData.c_str(), expectedEnqueueResult);

            useSize += expectedEnqueueResult;
            freeSize -= expectedEnqueueResult;
            break;
        }
        case eTestAction::Dequeue:
        {
            int dequeueSize = GetRandomNumber(1, size);
            int expectedDequeueResult;
            if (useSize >= dequeueSize)
            {
                expectedDequeueResult = dequeueSize;
            }
            else
            {
                expectedDequeueResult = 0;
            }
            string expectedData = referenceData.substr(0, expectedDequeueResult);
            ExpectEqual(rb.Dequeue(buf, dequeueSize), expectedDequeueResult, "Dequeue Result(Actual bytes)");
            string actualData(buf, expectedDequeueResult);
            ExpectEqual(actualData, expectedData, "Dequeued Result(Actual data)");
            referenceData.erase(0, expectedDequeueResult);

            useSize -= expectedDequeueResult;
            freeSize += expectedDequeueResult;
            break;
        }
        case eTestAction::Peek:
        {
            int peekSize = GetRandomNumber(1, size);
            int expectedPeekResult;
            if (useSize >= peekSize)
            {
                expectedPeekResult = peekSize;
            }
            else
            {
                expectedPeekResult =  0;
            }
            string expectedData = referenceData.substr(0, expectedPeekResult);
            ExpectEqual(rb.Peek(buf, peekSize), expectedPeekResult, "Peek Result(Actual bytes)");
            string actualData(buf, expectedPeekResult);
            ExpectEqual(actualData, expectedData, "Peek Result(Actual data)");
            break;
        }
        default:
            __debugbreak();
            break;
        }
    }
}

int main()
{
    int testCount;
    int successCount = 0;
    int failCount = 0;
    list<string> failList;
    cout << "Test Count: ";
    cin >> testCount;
    for (int i = 0; i < testCount; ++i)
    {
        cout << "\r" << OUT_DEFAULT << (format("Performing Test... ({} / {})", i, testCount)) << '\n';
        cout << "\r" << OUT_GREEN << "[PASS] : " << successCount << OUT_RED << " [FAIL] : " << failCount;

        try
        {
            Test(GetRandomNumber(10, 100));
        }
        catch (exception e)
        {
            failList.push_back(e.what());
            failCount++;
            cout << OUT_CURSOR_UP;
            continue;
        }

        successCount++;
        cout << OUT_CURSOR_UP;
    }

    cout << "\r" << OUT_CLEAR_LINE << OUT_DEFAULT << (format("Test Complete! ({} / {})", testCount, testCount)) << '\n';
    cout << "\r" << OUT_CLEAR_LINE << OUT_DEFAULT << "Test Result\n" << OUT_GREEN << "[PASS] : " << successCount << OUT_RED << " [FAIL] : " << failCount << '\n';
    cout << OUT_DEFAULT;
    int iter = failList.size();
    for (int i = 1; i <= iter; ++i)
    {
        cout << OUT_RED << "Failure Report " << i << "\n";
        cout << OUT_DEFAULT << failList.front() << "\n";
        failList.pop_front();
    }

    return 0;
}