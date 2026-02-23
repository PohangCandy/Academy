#include <cstdlib>   // system
#include <stdio.h>

int main()
{
    int i;
    char cmd[40];
    for (int i = 1; i < 255; i++) { 
        //sprintf_s(cmd, "ping 192.168.45.%d -n 1", i);
        sprintf_s(cmd, "ping 192.168.45.%d -n 1 | find \"TTL\"", i);
        system(cmd);
    }
}