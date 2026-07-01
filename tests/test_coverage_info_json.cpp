#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <star_terrain/file_data/CoverageInfo.hpp>
#include <star_terrain/file_data/CoverageInfo_json.hpp>

using namespace star::terrain;

TEST(CoverageInfoJson, ToJsonWritesXYKeysSwappedToConvention)
{
    // Internal: center.x = lat = 12.34, center.y = lon = 56.78
    // JSON convention: x = lon = 56.78, y = lat = 12.34
    CoverageInfo orig{.center = {12.34, 56.78}, .viewDistance = 1000};

    nlohmann::json j = orig;

    ASSERT_TRUE(j.contains("center"));
    EXPECT_TRUE(j["center"].contains("x"));
    EXPECT_TRUE(j["center"].contains("y"));
    EXPECT_FALSE(j["center"].contains("lat"));
    EXPECT_FALSE(j["center"].contains("lon"));
    EXPECT_DOUBLE_EQ(j["center"]["x"].get<double>(), 56.78);
    EXPECT_DOUBLE_EQ(j["center"]["y"].get<double>(), 12.34);
    EXPECT_EQ(j["view_distance"].get<int>(), 1000);
}

TEST(CoverageInfoJson, FromJsonReadsXYKeysSwappedToInternal)
{
    // JSON: x = lon = 56.78, y = lat = 12.34
    // Internal: center.x = lat = 12.34, center.y = lon = 56.78
    nlohmann::json j = nlohmann::json::parse(R"(
        {
            "center": {"x": 56.78, "y": 12.34},
            "view_distance": 2000
        }
    )");

    CoverageInfo parsed = j.get<CoverageInfo>();

    EXPECT_DOUBLE_EQ(parsed.center.x, 12.34);
    EXPECT_DOUBLE_EQ(parsed.center.y, 56.78);
    EXPECT_EQ(parsed.viewDistance, 2000);
}

TEST(CoverageInfoJson, RoundTripPreservesInternalData)
{
    CoverageInfo orig{.center = {-1.5, 42.0}, .viewDistance = 500};

    nlohmann::json j = orig;
    CoverageInfo parsed = j.get<CoverageInfo>();

    EXPECT_DOUBLE_EQ(parsed.center.x, orig.center.x);
    EXPECT_DOUBLE_EQ(parsed.center.y, orig.center.y);
    EXPECT_EQ(parsed.viewDistance, orig.viewDistance);
}