//-----------------------------------------------
// 프로젝트 명 : chrono를 사용한 시간 측정 프로젝트
// 
// 방법 : 소수 찾기 프로그램을 실행한 시간 측정
// 
// 예측 :
// 
// 결론 :
//-----------------------------------------------

#include <vector>
#include <iostream>
#include <chrono>

using namespace std;
const int MaxCount = 150000;

bool IsPrimeNumber(int number)
{
	if (number == 1)
		return false;
	if (number == 2 || number == 3)
		return true;
	for (int i = 2; i < number; i++)
	{
		if (number % i == 0)
			return false;
	}
	return true;
}

void PrintNumbers(const vector<int>& primes)
{
	for (int v : primes)
	{
		cout << v << endl;
	}
}

int main()
{
	vector<int> primes;

	auto t0 = chrono::system_clock::now();

	for (int i = 1; i <= MaxCount; i++)
	{
		if (IsPrimeNumber(i))
		{
			primes.push_back(i);
		}
	}

	auto t1 = chrono::system_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

	PrintNumbers(primes);

	cout << "Took" << duration << "milliseconds." << endl;

	

	return 0;
}