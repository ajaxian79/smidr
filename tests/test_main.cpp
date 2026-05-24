#include <cstdio>
#include <cstdlib>

extern int test_primitives();
extern int test_csg();
extern int test_document();

int main() {
    int failures = 0;

    std::printf("=== smidr tests ===\n\n");

    failures += test_primitives();
    failures += test_csg();
    failures += test_document();

    std::printf("\n=== %s (%d failure%s) ===\n",
                failures == 0 ? "PASS" : "FAIL",
                failures, failures == 1 ? "" : "s");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
