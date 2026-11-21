// A check of one platform's native side against the real store (CI builds it with
// native/linux/securestore.c or native/windows/securestore.c, a Mac with native/macos):
// set, read back, replace, remove, and whether the store says it is secure.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *secure_native_get(const char *key);
int secure_native_set(const char *key, const char *value);
int secure_native_remove(const char *key);
int secure_native_is_secure(void);

static int failed = 0;

static void check(int ok, const char *what) {
    printf("%s %s\n", ok ? "ok" : "FAIL", what);
    if (!ok) failed = 1;
}

static int is(char *got, const char *want) {
    int same = got && strcmp(got, want) == 0;
    free(got);
    return same;
}

int main(void) {
    const char *key = "eskiu-securestore-smoke";
    check(secure_native_is_secure(), "the store is secure");
    check(secure_native_set(key, "abc"), "set");
    check(is(secure_native_get(key), "abc"), "read back");
    check(secure_native_set(key, "d\xc3\xa9" "f"), "replace");
    check(is(secure_native_get(key), "d\xc3\xa9" "f"), "the new value");
    check(secure_native_set(key, ""), "an empty value");
    check(is(secure_native_get(key), ""), "empty read back");
    check(secure_native_remove(key), "remove");
    check(secure_native_get(key) == NULL, "gone");
    check(secure_native_remove(key), "remove again");
    printf(failed ? "FAIL native securestore\n" : "PASS native securestore\n");
    return failed;
}
