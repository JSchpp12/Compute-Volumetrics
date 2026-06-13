#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "service/detail/image_metric_manager/RayDistanceMetrics.hpp"
#include "service/detail/image_metric_manager/RayDistanceMetrics_json.hpp"

using namespace service::image_metric_manager;

TEST(RayDistanceMetricsJson, ToJsonAndBack)
{
    RayDistanceMetrics orig{{1.5, 0.5, 1.2, 100}, {2.5, 1.0, 2.1, 100}};

    nlohmann::json j = orig;

    ASSERT_TRUE(j.contains("includingInvalidRays"));
    ASSERT_TRUE(j.contains("excludingInvalidRays"));
    EXPECT_DOUBLE_EQ(j["includingInvalidRays"]["average"].get<double>(), 1.5);
    EXPECT_DOUBLE_EQ(j["includingInvalidRays"]["minimum"].get<double>(), 0.5);
    EXPECT_DOUBLE_EQ(j["includingInvalidRays"]["median"].get<double>(), 1.2);
    EXPECT_DOUBLE_EQ(j["excludingInvalidRays"]["average"].get<double>(), 2.5);
    EXPECT_DOUBLE_EQ(j["excludingInvalidRays"]["minimum"].get<double>(), 1.0);
    EXPECT_DOUBLE_EQ(j["excludingInvalidRays"]["median"].get<double>(), 2.1);
    EXPECT_EQ(j["includingInvalidRays"]["rayCount"].get<size_t>(), 100);
    EXPECT_EQ(j["excludingInvalidRays"]["rayCount"].get<size_t>(), 100);

    RayDistanceMetrics parsed = j.get<RayDistanceMetrics>();
    EXPECT_DOUBLE_EQ(parsed.includingInvalidRays.average, 1.5);
    EXPECT_DOUBLE_EQ(parsed.includingInvalidRays.minimum, 0.5);
    EXPECT_DOUBLE_EQ(parsed.includingInvalidRays.median, 1.2);
    EXPECT_DOUBLE_EQ(parsed.excludingInvalidRays.average, 2.5);
    EXPECT_DOUBLE_EQ(parsed.excludingInvalidRays.minimum, 1.0);
    EXPECT_DOUBLE_EQ(parsed.excludingInvalidRays.median, 2.1);
    EXPECT_EQ(parsed.includingInvalidRays.rayCount, 100);
    EXPECT_EQ(parsed.excludingInvalidRays.rayCount, 100);
}

TEST(RayDistanceMetricsJson, BackwardCompatibleMissingMedian)
{
    nlohmann::json j = nlohmann::json::parse(R"(
        {
            "includingInvalidRays": {"minimum": 0.5, "average": 1.5, "rayCount": 100},
            "excludingInvalidRays": {"minimum": 1.0, "average": 2.5, "rayCount": 100}
        }
    )");

    RayDistanceMetrics parsed = j.get<RayDistanceMetrics>();
    EXPECT_DOUBLE_EQ(parsed.includingInvalidRays.average, 1.5);
    EXPECT_DOUBLE_EQ(parsed.includingInvalidRays.minimum, 0.5);
    EXPECT_DOUBLE_EQ(parsed.includingInvalidRays.median, 0.0);
    EXPECT_DOUBLE_EQ(parsed.excludingInvalidRays.average, 2.5);
    EXPECT_DOUBLE_EQ(parsed.excludingInvalidRays.minimum, 1.0);
    EXPECT_DOUBLE_EQ(parsed.excludingInvalidRays.median, 0.0);
}
