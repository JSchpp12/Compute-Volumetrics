#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>
#include <cstring>   // std::strdup (optional if you prefer)
#include <cstdlib>   // EXIT_FAILURE

// Include your header
#include "util/CmdLine.hpp"

// Helper to construct (argc, argv) easily from a vector<string>
struct ArgvBuilder {
    std::vector<std::string> storage;
    std::vector<char*> ptrs;

    ArgvBuilder(std::initializer_list<std::string> init) : storage(init) {
        rebuild();
    }

    void rebuild() {
        ptrs.clear();
        for (auto& s : storage) ptrs.push_back(const_cast<char*>(s.c_str()));
        // GoogleTest doesn’t require argv to be null-terminated, but it’s conventional.
        ptrs.push_back(nullptr);
    }

    int argc() const { return static_cast<int>(storage.size()); }
    char** argv() { return ptrs.data(); }
};

// -------------------------
// TryGetArgValue() tests
// -------------------------

TEST(CmdLine_TryGetArgValue, ReturnsValueWhenPresent) {
    ArgvBuilder args = {"prog", "--terrain", "/data/terrain", "--controller", "cfg.json"};
    auto v = util::CmdLine::TryGetArgValue(args.argc(), args.argv(), std::string("--terrain"));
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "/data/terrain");

    auto c = util::CmdLine::TryGetArgValue(args.argc(), args.argv(), std::string("--controller"));
    ASSERT_TRUE(c.has_value());
    EXPECT_EQ(*c, "cfg.json");
}

TEST(CmdLine_TryGetArgValue, ReturnsNulloptWhenMissing) {
    ArgvBuilder args = {"prog", "--terrain", "/data/terrain"};
    auto v = util::CmdLine::TryGetArgValue(args.argc(), args.argv(), std::string("--controller"));
    EXPECT_FALSE(v.has_value());
}

TEST(CmdLine_TryGetArgValue, ReturnsNulloptWhenFlagIsLastNoValue) {
    ArgvBuilder args = {"prog", "--terrain"};
    auto v = util::CmdLine::TryGetArgValue(args.argc(), args.argv(), std::string("--terrain"));
    EXPECT_FALSE(v.has_value()); // no read-past-end
}

TEST(CmdLine_TryGetArgValue, FirstOccurrenceWinsWhenRepeated) {
    ArgvBuilder args = {"prog", "--terrain", "A", "--terrain", "B"};
    auto v = util::CmdLine::TryGetArgValue(args.argc(), args.argv(), std::string("--terrain"));
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "A"); // current implementation returns the first match
}

TEST(CmdLine_TryGetArgValue, AcceptsValueThatLooksLikeFlag) {
    // Current behavior: if the next token is "--something", it is treated as the value.
    ArgvBuilder args = {"prog", "--terrain", "--not-a-flag-value"};
    auto v = util::CmdLine::TryGetArgValue(args.argc(), args.argv(), std::string("--terrain"));
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "--not-a-flag-value");
}

// -----------------------------------------
// GetTerrainPath / GetSimControllerFilePath
// -----------------------------------------

TEST(CmdLine_GetTerrainPath, ReturnsPathWhenPresent) {
    ArgvBuilder args = {"prog", "--terrain", "/data/terrain"};
    auto path = util::CmdLine::GetTerrainPath(args.argc(), args.argv());
    EXPECT_EQ(path, "/data/terrain");
}

TEST(CmdLine_GetTerrainPath, ExitsWithMessageWhenMissing) {
#if GTEST_HAS_DEATH_TEST
    ArgvBuilder args = {"prog", "--controller", "cfg.json"};
    // stderr contains the error; match a substring via regex.
    EXPECT_EXIT(
        util::CmdLine::GetTerrainPath(args.argc(), args.argv()),
        ::testing::ExitedWithCode(EXIT_FAILURE),
        "Terrain dir must be provided with arg '--terrain'"
    );
#else
    GTEST_SKIP() << "Death tests not supported on this platform/config.";
#endif
}

TEST(CmdLine_GetSimControllerFilePath, ReturnsPathWhenPresent) {
    ArgvBuilder args = {"prog", "--controller", "cfg.json"};
    auto path = util::CmdLine::GetSimControllerFilePath(args.argc(), args.argv());
    EXPECT_EQ(path, "cfg.json");
}

TEST(CmdLine_GetSimControllerFilePath, ExitsWithMessageWhenMissing) {
#if GTEST_HAS_DEATH_TEST
    ArgvBuilder args = {"prog", "--terrain", "/data/terrain"};
    EXPECT_EXIT(
        util::CmdLine::GetSimControllerFilePath(args.argc(), args.argv()),
        ::testing::ExitedWithCode(EXIT_FAILURE),
        "Simulation controller path file must be provided with arg '--controller'"
    );
#else
    GTEST_SKIP() << "Death tests not supported on this platform/config.";
#endif
}