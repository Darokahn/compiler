#include <unistd.h>

#include "token.h"
#include "table/table.h"
#include "stateMachine.h"
#include "scanner.h"

void scanAndPrint(char* filename) {
    scanner_t s;
    scanner_init(&s, filename);
    token_t t;
    t.type = 0;
    while (t.type != END) {
        t = scanner_getNextToken(&s);
        if (t.type == BAD) break;
        token_prettyPrint(&t);
    }
}

void scanAndPrintDebug(char* filename) {
    scanner_t s;
    scanner_init(&s, filename);
    token_t t;
    t.type = 0;
    while (t.type != END) {
        t = scanner_getNextToken(&s);
        if (t.type == BAD) break;
        if (t.type == WHITESPACE || t.type == NEWLINE) token_prettyPrint(&t);
        else token_debugPrettyPrint(&t);
    }
}

int main() {
    // Here's a line comment
    /* Here's a block comment*/
    /* Here's
     * a
     * multiline
     * block
     * comment
     */
    /* Here's a block comment that ends strangely***/
    char c;
    scanAndPrint("main.c");
    read(0, &c, 1);
    scanAndPrintDebug("main.c");
} // Here's an EOF line comment
