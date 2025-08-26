#include <iostream>
using namespace std;

void insertValuebyByte(int pos, int value);
void printParameterbyByte(int p);
void printParameterbyx(int p);

unsigned int ui = 0;

int main()
{
	int pos;
	int value;

	while (1)
	{
		printf("위치 (1~4) : ");
		cin >> pos;
		printf("값 [0~255] : ");
		cin >> value;

		//해당 위치에 값을 넣는 함수
		insertValuebyByte(pos, value);

		//인수를 4바이트 단위로 출력하는 함수
		printParameterbyByte(ui);

		//전체 4바이트를 16진수로 출력하는 함수
		printParameterbyx(ui);
		printf("\n");
	}

	return 0;
}

//해당 위치에 값을 넣는 함수
void insertValuebyByte(int pos, int value)
{
	ui = _rotr(ui, (pos - 1) * 8);
	ui = ui & 0xffffff00;
	ui = ui | value;
	ui = _rotl(ui, (pos - 1) * 8);
}

//인수를 바이트 단위로 출력하는 함수
void printParameterbyByte(int p)
{
	for (int i = 0; i < 4; i++)
	{
		printf("%d 번째 바이트 값 : ", i + 1);
		printf("%d\n", ((p >> i * 8) & 0xff));
	}
	printf("\n");
}

//전체 4바이트를 16진수로 출력하는 함수
void printParameterbyx(int p)
{
	printf("전체 4바이트 값 : %p", p);
}