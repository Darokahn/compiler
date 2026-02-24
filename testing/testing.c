#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>

int test(char* dirname, char* cfilename, int len) {
    char fmt[] = "gcc %s/%s -fPIC -shared -o lib.so";
    char buffer[256 * 2 + sizeof fmt];
    sprintf(buffer, fmt, dirname, cfilename);
    int result = system(buffer);
    if (result != 0) return result;
    void* handle = dlopen("./lib.so", RTLD_NOW);
    if (handle == NULL) {
        fprintf(stderr, "could not open for symbols: %s\n", dlerror());
        return errno;
    }
    int (*init)() = dlsym(handle, "test_init");
    int testNum = init();
    printf("expecting %d tests from %s\n", testNum, cfilename);
    char testName[] = "test_****";
    for (int i = 0; i < testNum; i++) {
        snprintf(testName, sizeof testName, "test_%d", i);
        printf("expecting test %s\n", testName);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "list a directory for tests to be run in\n");
        return 1;
    }

    char* dirname = argv[1];
    DIR* dirFd = opendir(dirname);

    struct dirent* ent; 
    while ((ent = readdir(dirFd)) != NULL) {
        char* filename = ent->d_name;
        int len = strlen(filename);
        // good enough extension checking for me
        if (filename[len - 1] == 'c' && filename[len - 2] == '.') {
            int result = test(dirname, filename, len);
        }
    }
}
