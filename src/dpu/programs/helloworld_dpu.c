#include <stdio.h>

__host long long my_var[2];

int main() {
    my_var[0]+=my_var[1];
    printf("Done adding in the DPU\n");
}