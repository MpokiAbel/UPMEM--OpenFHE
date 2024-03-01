#include <stdio.h>
#include <defs.h>

int main() {
    printf("tasklet %u: stack = %u\n", me(), check_stack());
    return 0;
}