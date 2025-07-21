#include <chrono>
#include <fastlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class RotatingFileSinkTest : public ::testing::Test {
protected:
  // This function runs before each test case.
  void SetUp() override {
    // Create a unique temporary directory for each test case to avoid
    // conflicts. The path will be like
    // "/tmp/RotatingFileSinkTest_SomeUniqueName"
    _test_dir = fs::temp_directory_path() / "RotatingFileSinkTest";
    _test_dir += "_" + std::to_string(std::chrono::high_resolution_clock::now()
                                          .time_since_epoch()
                                          .count());

    fs::create_directory(_test_dir);
    _base_filepath = _test_dir / "test.log";
  }

  // This function runs after each test case, even if it fails.
  void TearDown() override {
    // Clean up the temporary directory and all its contents.
    std::error_code ec;
    fs::remove_all(_test_dir, ec);
  }

  // Helper function to read all lines from a file.
  std::vector<std::string> read_lines(const fs::path &path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file) {
      return lines; // Return empty vector if file can't be opened
    }
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }
    return lines;
  }

  fs::path _test_dir;
  fs::path _base_filepath;
};

// Test case 1: Basic logging without rotation.
TEST_F(RotatingFileSinkTest, LogWithoutRotation) {
  // Large max size, so no rotation should occur.
  fastlog::sinks::RotatingFileSink sink(_base_filepath.string(), 1024, 3);

  sink.log("Hello, world!");
  sink.log("Line 2.");

  // Always flush before asserting file content.
  sink.flush();

  ASSERT_TRUE(fs::exists(_base_filepath));
  auto lines = read_lines(_base_filepath);
  ASSERT_EQ(lines.size(), 2);
  EXPECT_EQ(lines[0], "Hello, world!");
  EXPECT_EQ(lines[1], "Line 2.");
}

// Test case 2: Trigger a single rotation.
TEST_F(RotatingFileSinkTest, SingleRotation) {
  // Max size is 20 bytes. Each log call writes ~10 bytes.
  fastlog::sinks::RotatingFileSink sink(_base_filepath.string(), 20, 3);

  sink.log("123456789"); // 9 bytes + 1 newline = 10 bytes
  sink.log("abcdefghi"); // 9 bytes + 1 newline = 10 bytes. Total = 20.

  // This next log should trigger rotation because current_size(20) +
  // next_size(4) > 20
  sink.log("XYZ"); // This should go into the new file.

  // Flush after all write operations.
  sink.flush();

  // Verify files
  auto rotated_path = _base_filepath;
  rotated_path += ".1";

  ASSERT_TRUE(fs::exists(_base_filepath)) << "New log file should exist.";
  ASSERT_TRUE(fs::exists(rotated_path)) << "Rotated log file .1 should exist.";

  // Check content of the new file
  auto new_lines = read_lines(_base_filepath);
  ASSERT_EQ(new_lines.size(), 1);
  EXPECT_EQ(new_lines[0], "XYZ");

  // Check content of the rotated file
  auto old_lines = read_lines(rotated_path);
  ASSERT_EQ(old_lines.size(), 2);
  EXPECT_EQ(old_lines[0], "123456789");
  EXPECT_EQ(old_lines[1], "abcdefghi");
}

// Test case 3: Trigger multiple rotations and check max files limit.
TEST_F(RotatingFileSinkTest, MaxFilesRotation) {
  // Max size 10 bytes, max 2 rotated files kept (test.log.1, test.log.2)
  fastlog::sinks::RotatingFileSink sink(_base_filepath.string(), 10, 2);

  sink.log("Log 1"); // size=6. File: test.log={"Log 1"}
  sink.log("Log 2"); // size=12. Rotates. New: {"Log 2"}, .1: {"Log 1"}
  sink.log("Log 3"); // size=12. Rotates. New: {"Log 3"}, .1: {"Log 2"}, .2:
                     // {"Log 1"}

  // Flush to persist the final state.
  sink.flush();

  auto file_current = _base_filepath;
  auto file_1 = fs::path(_base_filepath.string() + ".1");
  auto file_2 = fs::path(_base_filepath.string() + ".2");
  auto file_3 =
      fs::path(_base_filepath.string() + ".3"); // This should not exist

  ASSERT_TRUE(fs::exists(file_current));
  ASSERT_TRUE(fs::exists(file_1));
  ASSERT_TRUE(fs::exists(file_2));
  ASSERT_FALSE(fs::exists(file_3))
      << "Should not exceed max_files limit of 2 backups.";

  EXPECT_EQ(read_lines(file_current)[0], "Log 3");
  EXPECT_EQ(read_lines(file_1)[0], "Log 2");
  EXPECT_EQ(read_lines(file_2)[0], "Log 1");
}

// Test case 4: Constructor with invalid arguments.
TEST_F(RotatingFileSinkTest, InvalidArguments) {
  // max_files cannot be 0.
  ASSERT_THROW(
      fastlog::sinks::RotatingFileSink(_base_filepath.string(), 1024, 0),
      std::invalid_argument);
  // max_file_size cannot be 0.
  ASSERT_THROW(fastlog::sinks::RotatingFileSink(_base_filepath.string(), 0, 1),
               std::invalid_argument);
}