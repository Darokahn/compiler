#include <unistd.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "stringStreaming/stringStreaming.h"

#define CLR_RESET  "\x1b[0m"
#define CLR_RED    "\x1b[31m"
#define CLR_GREEN  "\x1b[32m"
#define CLR_CYAN   "\x1b[36m"
#define CLR_BOLD   "\x1b[1m"

int splitstream(FILE* pointerPair[2], char* fmt, ...) {
    va_list args0;
    va_start(args0, fmt);
    va_list args1;
    va_copy(args1, args0);
    vfprintf(pointerPair[0], fmt, args0);
    vfprintf(pointerPair[1], fmt, args1);
}


typedef char* heapString;

int main() {
    char symName[] = "test_************";
    int passed = 0;
    int failed = 0;
    int total = 0;

    linePrinter printer;
    linePrinter_init(&printer, "\n", "    ", stdout, (void*)fprintf);
    printer.tabCount = 3;

    for (int i = 0; true; i++) {
        snprintf(symName, sizeof symName, "test_%d", i);
        heapString (*test)(void*, outfunc) = dlsym(NULL, symName);

        if (test == NULL) {
            printf("\n" CLR_CYAN "[END OF SUITE]" CLR_RESET " Sequence halted: %s not found.\n", symName);
            break;
        }

        printf("\n" CLR_BOLD "--- RUNNING: %s ---" CLR_RESET "", symName);
        
        linePrinter_stream(&printer, "\n");
        heapString result = test(&printer, (void*)linePrinter_stream);
        fflush(stdout);

        if (result != NULL) {
            fprintf(stderr, CLR_RED "✖ FAILED: %s -> %s" CLR_RESET "\n", symName, result);
            free(result);
        } else {
            printf(CLR_GREEN "✔ PASSED: %s" CLR_RESET "\n", symName);
        }

        total++;
        failed += (result != NULL);
        passed += (result == NULL);
    }

    printf("\n" CLR_BOLD "==========================================" CLR_RESET "\n");
    printf(CLR_BOLD " TEST SUMMARY" CLR_RESET "\n");
    printf("  Total:  %d\n", total);
    printf("  Passed: %s%d %s" CLR_RESET "\n", passed == 0 ? "" : CLR_GREEN, passed, passed == 0 ? "" : "✔");
    printf("  Failed: %s%d %s" CLR_RESET "\n", failed == 0 ? "" : CLR_RED, failed, failed == 0 ? "" : "✖");
    printf(CLR_BOLD "==========================================" CLR_RESET "\n");

    return 0;
}
