#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "service/detail/image_metric_manager/VisibilityDistanceInfo.hpp"
#include "service/detail/image_metric_manager/VisibilityDistanceInfo_json.hpp"

using namespace service::image_metric_manager;

TEST(VisibilityDistanceInfoJson, RayMetricsToJsonAndBack)
{
    VisibilityDistanceInfo orig{.rayMetrics = RayVisibilityMetrics{{1.5, 0.5, 1.2, 100}, {2.5, 1.0, 2.1, 100}}};

    nlohmann::json j = orig;

    ASSERT_TRUE(j.contains("ray_metrics"));
    EXPECT_TRUE(j.contains("simple_distance") == false);
    EXPECT_DOUBLE_EQ(j["ray_metrics"]["includingInvalidRays"]["average"].get<double>(), 1.5);
    EXPECT_DOUBLE_EQ(j["ray_metrics"]["includingInvalidRays"]["minimum"].get<double>(), 0.5);
    EXPECT_DOUBLE_EQ(j["ray_metrics"]["includingInvalidRays"]["median"].get<double>(), 1.2);
    EXPECT_DOUBLE_EQ(j["ray_metrics"]["excludingInvalidRays"]["average"].get<double>(), 2.5);
    EXPECT_DOUBLE_EQ(j["ray_metrics"]["excludingInvalidRays"]["minimum"].get<double>(), 1.0);
    EXPECT_DOUBLE_EQ(j["ray_metrics"]["excludingInvalidRays"]["median"].get<double>(), 2.1);
    EXPECT_EQ(j["ray_metrics"]["includingInvalidRays"]["rayCount"].get<size_t>(), 100);
    EXPECT_EQ(j["ray_metrics"]["excludingInvalidRays"]["rayCount"].get<size_t>(), 100);

    VisibilityDistanceInfo parsed = j.get<VisibilityDistanceInfo>();
    ASSERT_TRUE(parsed.rayMetrics.has_value());
    EXPECT_TRUE(parsed.simpleDistance.has_value() == false);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.average, 1.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.minimum, 0.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.median, 1.2);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.average, 2.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.minimum, 1.0);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.median, 2.1);
    EXPECT_EQ(parsed.rayMetrics->includingInvalidRays.rayCount, 100);
    EXPECT_EQ(parsed.rayMetrics->excludingInvalidRays.rayCount, 100);
}

TEST(VisibilityDistanceInfoJson, SimpleDistanceToJsonAndBack)
{
    VisibilityDistanceInfo orig{.simpleDistance = 42.5};

    nlohmann::json j = orig;

    ASSERT_TRUE(j.contains("simple_distance"));
    EXPECT_TRUE(j.contains("ray_metrics") == false);
    EXPECT_DOUBLE_EQ(j["simple_distance"].get<double>(), 42.5);

    VisibilityDistanceInfo parsed = j.get<VisibilityDistanceInfo>();
    ASSERT_TRUE(parsed.simpleDistance.has_value());
    EXPECT_TRUE(parsed.rayMetrics.has_value() == false);
    EXPECT_DOUBLE_EQ(parsed.simpleDistance.value(), 42.5);
}

TEST(VisibilityDistanceInfoJson, BackwardCompatibleOldFormat)
{
    nlohmann::json j = nlohmann::json::parse(R"(
        {
            "includingInvalidRays": {"minimum": 0.5, "average": 1.5, "rayCount": 100},
            "excludingInvalidRays": {"minimum": 1.0, "average": 2.5, "rayCount": 100}
        }
    )");

    VisibilityDistanceInfo parsed = j.get<VisibilityDistanceInfo>();
    ASSERT_TRUE(parsed.rayMetrics.has_value());
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.average, 1.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.minimum, 0.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.median, 0.0);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.average, 2.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.minimum, 1.0);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.median, 0.0);
}

TEST(VisibilityDistanceInfoJson, BackwardCompatibleMissingMedian)
{
    nlohmann::json j = nlohmann::json::parse(R"(
        {
            "ray_metrics": {
                "includingInvalidRays": {"minimum": 0.5, "average": 1.5, "rayCount": 100},
                "excludingInvalidRays": {"minimum": 1.0, "average": 2.5, "rayCount": 100}
            }
        }
    )");

    VisibilityDistanceInfo parsed = j.get<VisibilityDistanceInfo>();
    ASSERT_TRUE(parsed.rayMetrics.has_value());
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.average, 1.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.minimum, 0.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->includingInvalidRays.median, 0.0);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.average, 2.5);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.minimum, 1.0);
    EXPECT_DOUBLE_EQ(parsed.rayMetrics->excludingInvalidRays.median, 0.0);
}