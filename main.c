#include <unistd.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef char* heapString;

int main() {
    char symName[] = "test_************";
    for (int i = 0; true; i++) {
        snprintf(symName, sizeof symName, "test_%d", i);
        heapString (*test)() = dlsym(NULL, symName);
        if (test == NULL) {
            printf("\n\nfinished testing; did not find %s\n\n", symName);
            break;
        }
        printf("\n\n##### TESTING %s #####\n\n", symName);
        heapString result = test();
        fflush(stdout);
        if (result != NULL) {
            fprintf(stderr, "test %s failed: %s\n", symName, result);
            free(result);
        }
    }
    return 0;
}
