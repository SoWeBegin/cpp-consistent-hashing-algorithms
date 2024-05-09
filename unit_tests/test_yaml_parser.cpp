#include <gtest/gtest.h>
#include "../YamlParser/YamlParser.h"


TEST(YamlParserConstructorTest, ValidYamlFile) {
    EXPECT_NO_THROW((YamlParser{ "./", "test.yaml" }));
}


TEST(YamlParserConstructorTest, NonExistentYamlFile) {
    EXPECT_THROW((YamlParser{ "../unit_tests", "nonexistent.yaml" }), std::invalid_argument);
    EXPECT_THROW((YamlParser{ "../unit_tests/", "invalid_test.yaml" }), std::runtime_error);
}


TEST(YamlParsingTest, ValidYamlFile) {
    YamlParser valid_yaml{ "../unit_tests/", "test.yaml" };

    const CommonSettings& commonSettings = valid_yaml.getCommonSettings();
    EXPECT_EQ(commonSettings.outputFolder, "/tmp");
    EXPECT_EQ(commonSettings.totalBenchmarkIterations, 5);
    EXPECT_EQ(commonSettings.unit, "NANOSECONDS");
    EXPECT_EQ(commonSettings.mode, "AverageTime");
    EXPECT_EQ(commonSettings.secondsForEachIteration, 5);
    EXPECT_EQ(commonSettings.numInitialActiveNodes.size(), 4);
    EXPECT_EQ(commonSettings.numInitialActiveNodes[0], 10);
    EXPECT_EQ(commonSettings.hashFunctions.size(), 4);
    EXPECT_EQ(commonSettings.hashFunctions[0], "murmur3");
    EXPECT_EQ(commonSettings.keyDistributions.size(), 3);
    EXPECT_EQ(commonSettings.keyDistributions[0], "uniform");

    const auto& algorithms = valid_yaml.getAlgorithms();
    EXPECT_EQ(algorithms.size(), 9);
    EXPECT_EQ(algorithms[0].name, "anchor");
    EXPECT_EQ(algorithms[0].args.at("capacity"), "10");

    const auto& benchmarks = valid_yaml.getBenchmarks();
    EXPECT_EQ(benchmarks.size(), 7);
    EXPECT_EQ(benchmarks[benchmarks.size() - 1].name, "monotonicity");
    EXPECT_EQ(benchmarks[benchmarks.size() - 1].args.size(), 2);
    EXPECT_EQ(benchmarks[benchmarks.size() - 1].args.at("keyMultiplier"), "100");
}

