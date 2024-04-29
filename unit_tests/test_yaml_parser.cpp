#include <gtest/gtest.h>
#include "../YamlParser/YamlParser.h"

TEST(YamlParserConstructorTest, ValidYamlFile) {
    EXPECT_NO_THROW((YamlParser{ "./", "test.yaml" }));
    YamlParser valid_yaml{ "./", "test.yaml" };
    const CommonSettings& commonSettings = valid_yaml.getCommonSettings();
    EXPECT_EQ(commonSettings.outputFolder, "/tmp");
    EXPECT_EQ(commonSettings.totalBenchmarkIterations, 5);
    EXPECT_EQ(commonSettings.unit, "NANOSECONDS");
    EXPECT_EQ(commonSettings.mode, "AverageTime");
    EXPECT_EQ(commonSettings.secondsForEachIteration, 5);
    EXPECT_EQ(commonSettings.numInitialActiveNodes.size(), 1);
    EXPECT_EQ(commonSettings.numInitialActiveNodes[0], 10000);
    EXPECT_EQ(commonSettings.hashFunctions.size(), 4);
    EXPECT_EQ(commonSettings.hashFunctions[0], "murmur3");
    EXPECT_EQ(commonSettings.keyDistributions.size(), 3);
    EXPECT_EQ(commonSettings.keyDistributions[0], "uniform");

    const auto& algorithms = valid_yaml.getAlgorithms();
    EXPECT_EQ(algorithms.size(), 6);

    const auto& benchmarks = valid_yaml.getBenchmarks();
    EXPECT_TRUE(benchmarks.empty());
}

/*
TEST_F(YamlParserConstructorTest, NonExistentYamlFile) {
    const CommonSettings& commonSettings = non_existent_yaml.getCommonSettings();
    EXPECT_EQ(commonSettings.outputFolder, "/tmp");
    EXPECT_EQ(commonSettings.totalBenchmarkIterations, 5);
    EXPECT_EQ(commonSettings.unit, "NANOSECONDS");
    EXPECT_EQ(commonSettings.mode, "AverageTime");
    EXPECT_EQ(commonSettings.secondsForEachIteration, 5);
    EXPECT_TRUE(commonSettings.numInitialActiveNodes.empty());
    EXPECT_TRUE(commonSettings.hashFunctions.empty());
    EXPECT_TRUE(commonSettings.keyDistributions.empty());

    const auto& algorithms = non_existent_yaml.getAlgorithms();
    EXPECT_TRUE(algorithms.empty());

    const auto& benchmarks = non_existent_yaml.getBenchmarks();
    EXPECT_TRUE(benchmarks.empty());
}

// Test case to verify the behavior when the YAML file cannot be opened
TEST_F(YamlParserConstructorTest, CannotOpenYamlFile) {
    const CommonSettings& commonSettings = unopeable_yaml.getCommonSettings();
    EXPECT_EQ(commonSettings.outputFolder, "/tmp");
    EXPECT_EQ(commonSettings.totalBenchmarkIterations, 5);
    EXPECT_EQ(commonSettings.unit, "NANOSECONDS");
    EXPECT_EQ(commonSettings.mode, "AverageTime");
    EXPECT_EQ(commonSettings.secondsForEachIteration, 5);
    EXPECT_TRUE(commonSettings.numInitialActiveNodes.empty());
    EXPECT_TRUE(commonSettings.hashFunctions.empty());
    EXPECT_TRUE(commonSettings.keyDistributions.empty());

    const auto& algorithms = unopeable_yaml.getAlgorithms();
    EXPECT_TRUE(algorithms.empty());

    const auto& benchmarks = unopeable_yaml.getBenchmarks();
    EXPECT_TRUE(benchmarks.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
*/