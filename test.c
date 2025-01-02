#include "callater.h"
#include <stdio.h>
#include <stdlib.h>

void exitPrgrm(void *arg)
{
    (void)arg;
    puts("OUTS");
    exit(69420);
}

void printNum(void *num)
{
    printf("%d\n", *(int*)num);
}

int main()
{
    CallaterInit();
    
    int *nums = malloc(1000 * sizeof(int));
    for(int i = 0 ; i < 1000 ; i++)
    {
        nums[i] = i;
        Invoke(printNum, &nums[i], i);
    }
    
    while(1)
    {
        CallaterUpdate();
    }
}
