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
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        token_prettyPrint(&t);
    }
}

void scanAndPrintDebug(char* filename) {
    scanner_t s;
    scanner_init(&s, filename);
    token_t t;
    t.type = 0;
    printf("%-3d", s.lineCount);
    while (t.type != eof_token) {
        t = scanner_getNextToken(&s);
        if (t.type == bad_token) break;
        if (t.type == whitespace_token) {
            token_prettyPrint(&t);
        }
        else if (t.type == newline_token) {
            token_prettyPrint(&t);
            printf("%-3d", s.lineCount);
        }
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
    /**/
    char c_1;
    scanAndPrint("main.c");
    read(0, &c_1, 1);
    scanAndPrintDebug("test.c");
    printf("\n");
} // Here's an EOF line comment
