/** @defgroup Testing
 *
 * @brief Reusable unit test infrastructure.
 *
 * This module defines header-only infra for unit tests that can be used to
 * integrate tests directly where the implementation is. All you really need
 * to do then is enable a macro, include the file you're testing and compile
 * into an executable.
 *
 * @{
 */

/** @file unit_test.hh
 *
 * @brief The unit test infrastructure implementation.
 *
 * Include this at the very beginning in the files you want to define unit
 * tests in. It has no dependencies within libostd.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_UNIT_TEST_HH
#define OSTD_UNIT_TEST_HH

#ifdef OSTD_BUILD_TESTS
#include <cstdio>
#include <cstddef>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#endif

namespace ostd {
namespace test {

/** @addtogroup Testing
 * @{
 */

#ifdef OSTD_BUILD_TESTS

#define OSTD_TEST_MODULE_STRINGIFY(x) #x
#define OSTD_TEST_MODULE_STR(x) OSTD_TEST_MODULE_STRINGIFY(x)
#define OSTD_TEST_MODULE OSTD_TEST_MODULE_STR(OSTD_BUILD_TESTS)

static std::vector<void (*)()> test_cases;

static bool add_test(std::string testn, void (*func)()) {
    if (testn == OSTD_TEST_MODULE) {
        test_cases.push_back(func);
    }
    return true;
}

#define OSTD_UNIT_TEST(module, body) \
static bool test_case_##module##_##__LINE__ = \
    ostd::test::add_test(#module, []() { using namespace ostd::test; body });

struct test_error {};

void fail_if(bool b) {
    if (b) {
        throw test_error{};
    }
}

void fail_if_not(bool b) {
    if (!b) {
        throw test_error{};
    }
}

void run() {
    int succ = 0, fail = 0;
    for (auto &f: test_cases) {
        try {
            f();
        } catch (test_error) {
            ++fail;
            continue;
        }
        ++succ;
    }
#if defined(_WIN32) || defined(WIN32)
    printf(
        "%s...\t%d out of %d (%d failures)\n",
        OSTD_TEST_MODULE, succ, succ + fail, fail
    );
#else
    printf(
        "%s...\t%s\033[1m%d out of %d\033[0m (%d failures)\n",
        OSTD_TEST_MODULE, fail ? "\033[91m" : "\033[92m",
        succ, succ + fail, fail
    );
#endif
}

#else /* OSTD_BUILD_TESTS */

#define OSTD_UNIT_TEST(module, body)

#endif /* OSTD_BUILD_TESTS */

/** @} */

} /* namespace test */
} /* namespace ostd */

#endif

/** @} */
