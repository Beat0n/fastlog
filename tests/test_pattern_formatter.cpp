#include <fastlog/formatters/pattern_formatter.h>
#include <fastlog/log_message.h>
#include <gtest/gtest.h>
#include <chrono>

class PatternFormatterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Prepare a sample LogMessage for all tests
    msg.timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());  // Timestamp can vary, so we won't
                                                       // assert its exact value
    msg.thread_id = std::this_thread::get_id();
    msg.level = fastlog::LogLevel::Info;
    msg.payload = "This is the payload.";
    msg.location = std::source_location::current();
  }

  std::string format(const std::string &pattern) {
    fastlog::formatters::PatternFormatter formatter(pattern);
    std::string result;
    formatter.format(msg, result);
    return result;
  }

  fastlog::LogMessage msg;
};

TEST_F(PatternFormatterTest, FormatsLiteralText) {
  EXPECT_EQ(format("Hello world"), "Hello world");
  EXPECT_EQ(format("  leading and trailing spaces  "), "  leading and trailing spaces  ");
}

TEST_F(PatternFormatterTest, FormatsEscapedPercent) {
  EXPECT_EQ(format("%%"), "%");
  EXPECT_EQ(format("Rate is 100%%"), "Rate is 100%");
}

TEST_F(PatternFormatterTest, FormatsLogLevel) {
  msg.level = fastlog::LogLevel::Debug;
  EXPECT_EQ(format("[%l]"), "[DEBUG]");
  msg.level = fastlog::LogLevel::Error;
  EXPECT_EQ(format("[%l]"), "[ERROR]");
}

TEST_F(PatternFormatterTest, FormatsPayload) {
  msg.payload = "A custom message.";
  EXPECT_EQ(format("%v"), "A custom message.");
  EXPECT_EQ(format("Message: %v"), "Message: A custom message.");
}

TEST_F(PatternFormatterTest, FormatsThreadId) {
  // We can't assert the exact thread_id, but we can check if it's formatted.
  std::stringstream ss;
  ss << msg.thread_id;
  EXPECT_EQ(format("%t"), ss.str());
}

TEST_F(PatternFormatterTest, FormatsTimestamp) {
  // We test timestamp parts individually
  // This requires a bit of work to get the expected values
  auto timestamp_str = format("%Y-%m-%d %H:%M:%S");
  // Check if the format is plausible (e.g., "2023-10-27 10:30:15.123456")
    EXPECT_EQ(timestamp_str.length(), 26); // YYYY-MM-DD HH:MM:SS.ffffff
  EXPECT_EQ(timestamp_str[4], '-');
  EXPECT_EQ(timestamp_str[7], '-');
  EXPECT_EQ(timestamp_str[19], '.');
}

TEST_F(PatternFormatterTest, FormatsComplexPattern) {
  msg.level = fastlog::LogLevel::Warn;
  msg.payload = "Disk is 85% full.";

  // We can't assert timestamp and thread_id fully, so we check the static
  // parts.
  auto result = format("[%Y-%m-%d %H:%M:%S] [%l] [%t] %v");

  std::stringstream ss;
  ss << msg.thread_id;
  std::string expected_ending = " [WARN] [" + ss.str() + "] Disk is 85% full.";

  // Check that the result string ends with the expected pattern
  ASSERT_GE(result.length(), expected_ending.length());
  EXPECT_EQ(result.substr(result.length() - expected_ending.length()), expected_ending);
}