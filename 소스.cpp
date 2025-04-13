
#include <iostream>
using namespace std;

int main()
{
	unsigned char input = 0x28;

	printf("%d의 바이너리 : ", input);

	//1과 비교해서 둘 다 1인 경우만 보여주면 될거같은데?
	for (int i = 0; i < 8; i++)
	{
		if (input & 0x80)
		{
			printf("1");
		}
		else
		{
			printf("0");
		}
		input = input << 1;
	}

	cout << "\n";


	return 0;
}