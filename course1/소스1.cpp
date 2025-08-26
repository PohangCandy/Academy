#include <iostream>
using namespace std;

int main()
{
	unsigned short us = 0;

	int bitpos;
	int se;

	while (1)
	{
		printf("비트위치 : ");
		cin >> bitpos;
		printf("OFF/ON [0 , 1] : ");
		cin >> se;

		if (se)
		{
			se = se << (bitpos - 1);
			us = us | se;
		}
		else
		{
			us = _rotr16(us, bitpos - 1);
			us = us & 0xfffe;
			us = _rotl16(us, bitpos - 1);
		}

		for (int i = 16; i > 0; i--)
		{
			if ((us >> (i - 1)) & 1)
			{
				printf("%d 번 비트 : ON", i);
			}
			else
			{
				printf("%d 번 비트 : OFF", i);
			}
			cout << "\n";
			
		}
		cout << "\n";

	}

	return 0;
}