#include <gtest/gtest.h>

#include "config/AppConfigLoader.hpp"
#include "config/AppConfigInfo.hpp"

#include <starlight/core/Exceptions.hpp>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace
{
struct ArgvBuilder
{
    std::vector<std::string> storage;
    std::vector<char *> ptrs;

    ArgvBuilder(std::initializer_list<std::string> init) : storage(init) { rebuild(); }

    void rebuild()
    {
        ptrs.clear();
        for (auto &s : storage)
            ptrs.push_back(const_cast<char *>(s.c_str()));
        ptrs.push_back(nullptr);
    }

    int argc() const { return static_cast<int>(storage.size()); }
    char **argv() { return ptrs.data(); }
};

class AppConfigLoader_CreateIfMissing : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_tempDir = std::filesystem::temp_directory_path() / "appconfig_loader_test";
        std::filesystem::remove_all(m_tempDir);
        std::filesystem::create_directories(m_tempDir);
    }

    void TearDown() override { std::filesystem::remove_all(m_tempDir); }

    std::filesystem::path m_tempDir;
};
} // namespace

TEST_F(AppConfigLoader_CreateIfMissing, CreatesDefaultFileWhenPathDoesNotExist)
{
    const auto cfgPath = m_tempDir / "nonexistent_config.json";
    ASSERT_FALSE(std::filesystem::exists(cfgPath));

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};

    auto [cfg, status] = config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv());

    EXPECT_EQ(status, config::LoadStatus::CreatedDefault);
    ASSERT_TRUE(std::filesystem::exists(cfgPath)) << "Expected default config file to be created";

    std::ifstream file(cfgPath);
    ASSERT_TRUE(file.is_open());
    nlohmann::json j;
    file >> j;

    EXPECT_TRUE(j.contains("volumeName"));
    EXPECT_TRUE(j.contains("terrainDir"));
    EXPECT_TRUE(j.contains("simControllerPath"));
    EXPECT_TRUE(j.contains("enableDistanceMarkers"));
    EXPECT_TRUE(j.contains("enableCutoffHighlighting"));
    EXPECT_TRUE(j.contains("interactiveConfig"));
    EXPECT_TRUE(j["interactiveConfig"].contains("cameraMovementSpeed"));
    EXPECT_TRUE(j["interactiveConfig"].contains("cameraSensitivity"));
    EXPECT_TRUE(j["interactiveConfig"].contains("objectMovementSpeed"));

    EXPECT_EQ(j["volumeName"].get<std::string>(), "");
    EXPECT_FLOAT_EQ(j["interactiveConfig"]["cameraMovementSpeed"].get<float>(), 5000.0f);
    EXPECT_FLOAT_EQ(j["interactiveConfig"]["cameraSensitivity"].get<float>(), 0.1f);
    EXPECT_FLOAT_EQ(j["interactiveConfig"]["objectMovementSpeed"].get<float>(), 300.0f);
}

TEST_F(AppConfigLoader_CreateIfMissing, DoesNotCreateFileWhenAppConfigFlagAbsent)
{
    const auto cfgPath = m_tempDir / "should_not_be_created.json";
    ASSERT_FALSE(std::filesystem::exists(cfgPath));

    ArgvBuilder args = {"prog"};
    auto [cfg, status] = config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv());

    EXPECT_NE(status, config::LoadStatus::CreatedDefault);
    EXPECT_FALSE(std::filesystem::exists(cfgPath)) << "No file should be created when --appConfig is absent";
}

class AppConfigLoader_PatchMissing : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_tempDir = std::filesystem::temp_directory_path() / "appconfig_patch_test";
        std::filesystem::remove_all(m_tempDir);
        std::filesystem::create_directories(m_tempDir);
    }

    void TearDown() override { std::filesystem::remove_all(m_tempDir); }

    nlohmann::json buildCompleteConfigJson() const
    {
        nlohmann::json j;
        j["volumeName"] = "myvol";
        j["terrainDir"] = (m_tempDir / "nonexistent_terrain").string();
        j["simControllerPath"] = "/some/path";
        j["enableDistanceMarkers"] = true;
        j["enableCutoffHighlighting"] = true;
        j["interactiveConfig"] = nlohmann::json{{"cameraMovementSpeed", 999.0f},
                                                {"cameraSensitivity", 2.5f},
                                                {"objectMovementSpeed", 42.0f}};
        return j;
    }

    static nlohmann::json readJsonFile(const std::filesystem::path &path)
    {
        std::ifstream file(path);
        nlohmann::json j;
        file >> j;
        return j;
    }

    static void writeJsonFile(const std::filesystem::path &path, const nlohmann::json &j)
    {
        std::ofstream out(path);
        out << j.dump(4);
    }

    std::filesystem::path m_tempDir;
};

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_volumeName)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j.erase("volumeName");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched.contains("volumeName"));
    EXPECT_EQ(patched["volumeName"].get<std::string>(), "");
    EXPECT_EQ(patched["terrainDir"].get<std::string>(), (m_tempDir / "nonexistent_terrain").string());
    EXPECT_TRUE(patched["enableDistanceMarkers"].get<bool>());
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_terrainDir)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j.erase("terrainDir");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched.contains("terrainDir"));
    EXPECT_EQ(patched["terrainDir"].get<std::string>(), "");
    EXPECT_EQ(patched["volumeName"].get<std::string>(), "myvol");
    EXPECT_TRUE(patched["enableCutoffHighlighting"].get<bool>());
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_simControllerPath)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j.erase("simControllerPath");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched.contains("simControllerPath"));
    EXPECT_EQ(patched["simControllerPath"].get<std::string>(), "");
    EXPECT_EQ(patched["volumeName"].get<std::string>(), "myvol");
    EXPECT_TRUE(patched["enableDistanceMarkers"].get<bool>());
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_enableDistanceMarkers)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j.erase("enableDistanceMarkers");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched.contains("enableDistanceMarkers"));
    EXPECT_FALSE(patched["enableDistanceMarkers"].get<bool>());
    EXPECT_EQ(patched["volumeName"].get<std::string>(), "myvol");
    EXPECT_TRUE(patched["enableCutoffHighlighting"].get<bool>());
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_enableCutoffHighlighting)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j.erase("enableCutoffHighlighting");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched.contains("enableCutoffHighlighting"));
    EXPECT_FALSE(patched["enableCutoffHighlighting"].get<bool>());
    EXPECT_EQ(patched["volumeName"].get<std::string>(), "myvol");
    EXPECT_TRUE(patched["enableDistanceMarkers"].get<bool>());
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_interactiveConfig)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j.erase("interactiveConfig");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched.contains("interactiveConfig"));
    EXPECT_TRUE(patched["interactiveConfig"].contains("cameraMovementSpeed"));
    EXPECT_TRUE(patched["interactiveConfig"].contains("cameraSensitivity"));
    EXPECT_TRUE(patched["interactiveConfig"].contains("objectMovementSpeed"));
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraMovementSpeed"].get<float>(), 5000.0f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraSensitivity"].get<float>(), 0.1f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["objectMovementSpeed"].get<float>(), 300.0f);
    EXPECT_EQ(patched["volumeName"].get<std::string>(), "myvol");
    EXPECT_TRUE(patched["enableDistanceMarkers"].get<bool>());
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_cameraMovementSpeed)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j["interactiveConfig"].erase("cameraMovementSpeed");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched["interactiveConfig"].contains("cameraMovementSpeed"));
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraMovementSpeed"].get<float>(), 5000.0f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraSensitivity"].get<float>(), 2.5f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["objectMovementSpeed"].get<float>(), 42.0f);
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_cameraSensitivity)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j["interactiveConfig"].erase("cameraSensitivity");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched["interactiveConfig"].contains("cameraSensitivity"));
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraSensitivity"].get<float>(), 0.1f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraMovementSpeed"].get<float>(), 999.0f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["objectMovementSpeed"].get<float>(), 42.0f);
}

TEST_F(AppConfigLoader_PatchMissing, PatchesMissing_objectMovementSpeed)
{
    const auto cfgPath = m_tempDir / "config.json";
    auto j = buildCompleteConfigJson();
    j["interactiveConfig"].erase("objectMovementSpeed");
    writeJsonFile(cfgPath, j);

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    ASSERT_TRUE(patched["interactiveConfig"].contains("objectMovementSpeed"));
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["objectMovementSpeed"].get<float>(), 300.0f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraMovementSpeed"].get<float>(), 999.0f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraSensitivity"].get<float>(), 2.5f);
}

TEST_F(AppConfigLoader_PatchMissing, PatchesAllFieldsWhenFileIsEmptyObject)
{
    const auto cfgPath = m_tempDir / "config.json";
    writeJsonFile(cfgPath, nlohmann::json::object());

    ArgvBuilder args = {"prog", "--appConfig", cfgPath.string()};
    EXPECT_THROW(config::AppConfigLoader::LoadFromArgs(args.argc(), args.argv()), star::core::RuntimeError);

    auto patched = readJsonFile(cfgPath);
    EXPECT_EQ(patched["volumeName"].get<std::string>(), "");
    EXPECT_EQ(patched["terrainDir"].get<std::string>(), "");
    EXPECT_EQ(patched["simControllerPath"].get<std::string>(), "");
    EXPECT_FALSE(patched["enableDistanceMarkers"].get<bool>());
    EXPECT_FALSE(patched["enableCutoffHighlighting"].get<bool>());
    ASSERT_TRUE(patched.contains("interactiveConfig"));
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraMovementSpeed"].get<float>(), 5000.0f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["cameraSensitivity"].get<float>(), 0.1f);
    EXPECT_FLOAT_EQ(patched["interactiveConfig"]["objectMovementSpeed"].get<float>(), 300.0f);
}