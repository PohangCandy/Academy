#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <conio.h>
#include <string.h>

struct sT_Item {
    char name[100];
    int percent;
};

sT_Item Gatcha[] = {
{"칼",300},
{"방패",200},
{"레어템",100},
{"초레어템",1},
};

int GetMaxItemNum()
{
    int sum = 0;
    for (int i = 0; i < sizeof(Gatcha) / sizeof(Gatcha[0]); i++)
    {
        sum += Gatcha[i].percent;
    }

    return sum;
}

void Gacha()
{
    static int TryNum = 0;

    int r = (rand() % GetMaxItemNum()) + 1;

    char name[100];
    int sum = 0;
    for (int i = 0; i < sizeof(Gatcha) / sizeof(Gatcha[0]); i++)
    {
        sum += Gatcha[i].percent;
        if (r <= sum)
        {
            strcpy(name, Gatcha[i].name);
            break;
        }
    }
    TryNum++;

    printf("%d \n", r);
    printf("횟수: %d 아이템 이름: %s\n", TryNum, name);
    
}

int main()
{
    srand(time(NULL));

    while (1)
    {
        if (_kbhit())
        {
            char ch = _getch();
            if (ch == ' ') {
                Gacha();
            }
            else if (ch == 'q')
            {
                break;
            }
        }
    }
    

    return 0;
}
