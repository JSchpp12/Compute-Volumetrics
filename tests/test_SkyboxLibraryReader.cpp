#include <gtest/gtest.h>

#include <star_skybox/io/SkyboxLibraryReader.hpp>

#include <starlight/job/tasks/IOTask.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
namespace fs = std::filesystem;

struct TempDirectory
{
    TempDirectory()
    {
        const auto stamp = static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        m_path = fs::temp_directory_path() /
                 ("star_skybox_library_test_" + std::to_string(stamp) + "_" +
                  std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        fs::create_directories(m_path);
    }

    ~TempDirectory()
    {
        std::error_code ec;
        fs::remove_all(m_path, ec);
    }

    fs::path m_path;
};

void WriteFile(const fs::path &path, std::string_view content = {})
{
    std::ofstream stream(path, std::ios::binary);
    stream << content;
}

star::skybox::io::SkyboxLibraryResult ScanDirectory(const fs::path &directory)
{
    star::skybox::io::SkyboxLibraryReader reader;
    auto future = reader.getFuture();

    // Exercise the same payload path the app uses: resolve the directory, then invoke the reader.
    auto payload = star::job::tasks::io::ReadPayload<star::skybox::io::SkyboxLibraryReader>{
        .filePath = directory, .readFunction = std::move(reader), .kind = star::io::PathKind::directory};
    payload();

    return future.get();
}
} // namespace

TEST(SkyboxLibraryReader, PairsSidecarsWithExrAndSorts)
{
    TempDirectory temp;

    WriteFile(temp.m_path / "a.exr");
    WriteFile(temp.m_path / "a.skyboxinfo.json", R"({"name":"Zulu"})");
    WriteFile(temp.m_path / "a.exr.star.json", "{}");
    WriteFile(temp.m_path / "b.exr");
    WriteFile(temp.m_path / "b.skyboxinfo.json", R"({"name":"Alpha"})");
    WriteFile(temp.m_path / "foo.bar.exr");
    WriteFile(temp.m_path / "foo.bar.skyboxinfo.json", R"({"layout":"equirect"})");
    WriteFile(temp.m_path / "orphan.skyboxinfo.json", R"({"name":"Orphan"})");
    WriteFile(temp.m_path / "image_only.exr");
    WriteFile(temp.m_path / "malformed.exr");
    WriteFile(temp.m_path / "malformed.skyboxinfo.json", "{ not json");

    const auto result = ScanDirectory(temp.m_path);
    ASSERT_TRUE(result.has_value()) << (result ? std::string{} : result.error());

    const auto &entries = *result;
    ASSERT_EQ(entries.size(), 4u);

    EXPECT_EQ(entries[0].displayName, "Alpha");
    EXPECT_EQ(entries[0].texturePath.filename().string(), "b.exr");
    EXPECT_EQ(entries[1].displayName, "Zulu");
    EXPECT_EQ(entries[1].texturePath.filename().string(), "a.exr");
    EXPECT_EQ(entries[2].displayName, "foo.bar");
    EXPECT_EQ(entries[2].texturePath.filename().string(), "foo.bar.exr");
    EXPECT_EQ(entries[3].displayName, "malformed");
    EXPECT_EQ(entries[3].texturePath.filename().string(), "malformed.exr");

    for (const auto &entry : entries)
        EXPECT_TRUE(entry.texturePath.is_absolute());
}

TEST(SkyboxLibraryReader, MissingDirectoryReportsError)
{
    const auto result = ScanDirectory(fs::path{"__star_missing_skybox_dir__"});
    EXPECT_FALSE(result.has_value());
}

TEST(SkyboxLibraryReader, EmptyDirectoryReturnsNoEntries)
{
    TempDirectory temp;

    const auto result = ScanDirectory(temp.m_path);
    ASSERT_TRUE(result.has_value()) << (result ? std::string{} : result.error());
    EXPECT_TRUE(result->empty());
}
