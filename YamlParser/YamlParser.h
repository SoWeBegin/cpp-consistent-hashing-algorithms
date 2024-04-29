#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include "yaml-cpp/parser.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <iostream>

struct AlgorithmSettings final {
  std::string name;
  std::unordered_map<std::string, std::string>
      args; // [argumentName][argumentValue]
};

// specified values are default ones
struct CommonSettings final {
  std::string outputFolder = "/tmp";
  std::size_t totalBenchmarkIterations = 5;
  std::string unit = "NANOSECONDS";
  std::string mode = "AverageTime";
  std::size_t secondsForEachIteration = 5;
  std::vector<std::size_t> numInitialActiveNodes;
  std::vector<std::string> hashFunctions;
  std::vector<std::string> keyDistributions;
};

struct BenchmarkSettings final {
  std::string name;
  CommonSettings commonSettings;
  std::unordered_map<std::string, std::string>
      args; // [argumentName][argumentValue]
};

class YamlParser final {
private:
  CommonSettings m_commonSettings;
  std::vector<AlgorithmSettings> m_algorithms;
  std::vector<BenchmarkSettings> m_benchmarks;

public:
  YamlParser() = default;

  explicit YamlParser(const std::filesystem::path &directory,
                      const std::string &fileName) {
    const std::filesystem::path filePath = fileName; // directory / fileName;

    if (!std::filesystem::exists(filePath)) {
      throw std::invalid_argument(
          "[YamlParser::YamlParser]: Yaml file not found!\n");
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
      throw std::invalid_argument(
          "[YamlParser::YamlParser]: Unable to open yaml file!\n");
      return;
    }

    const auto m_config = YAML::LoadFile(filePath.string());
    if (m_config) {
      parseCommon(m_config["common"], m_commonSettings);
      parseAlgorithms(m_config["algorithms"]);
      parseBenchmarks(m_config["benchmarks"]);
    } else {
      throw std::runtime_error(
          "[YamlParser::YamlParser]: Error loading Yaml file!\n");
    }
  }

  void parseBenchmarks(const YAML::Node &benchmarks) {
    if (benchmarks) {
      for (const auto &benchmark : benchmarks) {
        if (benchmark["name"]) {
          BenchmarkSettings benchmarkSettings{
              benchmark["name"].as<std::string>(), m_commonSettings};

          if (benchmark["common"]) {
            parseCommon(benchmark["common"], benchmarkSettings.commonSettings,
                        true);
          }

          if (benchmark["args"]) {
            for (const auto &pair : benchmark["args"]) {
              const YAML::Node &key = pair.first;
              const YAML::Node &value = pair.second;

              if (key.IsScalar()) {
                const std::string argName = key.as<std::string>();
                std::string argValue;

                if (value.IsScalar()) {
                  argValue = value.as<std::string>();
                } else { // otherwise .as<std::string> throws a bad conversion
                         // exception
                  std::stringstream ss;
                  ss << value;
                  argValue = ss.str();
                }
                benchmarkSettings.args[argName] = argValue;
              }
            }
          }
          m_benchmarks.push_back(benchmarkSettings);
        }
      }
    }
  }

  void parseAlgorithms(const YAML::Node &algorithms) {
    if (algorithms) {
      for (const auto &algorithm : algorithms) {
        if (algorithm["name"]) {
          AlgorithmSettings algorithmSettings{
              algorithm["name"].as<std::string>()};

          if (algorithm["args"]) {
            for (const auto &pair : algorithm["args"]) { // [key][value]
              const std::string argName = pair.first.as<std::string>();
              const std::string argValue = pair.second.as<std::string>();
              algorithmSettings.args[argName] = argValue;
            }
          }
          m_algorithms.push_back(algorithmSettings);
        }
      }
    }
  }

  void parseCommon(const YAML::Node &common, CommonSettings &commonSettings,
                   bool is_benchmark_override = false) {
    if (common) {
      if (common["output-folder"]) {
        commonSettings.outputFolder = common["output-folder"].as<std::string>();
      }
      if (common["iterations"]) {
        auto &iterations = common["iterations"];
        if (iterations["execution"]) {
          commonSettings.totalBenchmarkIterations =
              iterations["execution"].as<std::size_t>();
        }
      }
      if (common["time"]) {
        auto &time = common["time"];
        if (time["unit"]) {
          const std::string time_unit = time["unit"].as<std::string>();
          if (time_unit != "SECONDS" && time_unit != "MILLISECONDS" &&
              time_unit != "MICROSECONDS" && time_unit != "NANOSECONDS") {
            fmt::println("time-unit has a wrong value in the yaml file. "
                         "Proceeding with default time-unit = NANOSECONDS");
          } else {
            commonSettings.unit = time_unit;
          }
        }
        if (time["mode"]) {
          const std::string time_mode = time["mode"].as<std::string>();
          if (time_mode != "AverageTime" && time_mode != "SampleTime" &&
              time_mode != "SingleShotTime" && time_mode != "Throughput" &&
              time_mode != "ALL") {
            fmt::println("time-mode has a wrong value in the yaml file. "
                         "Proceeding with default time-mode = AverageTime");
          } else {
            commonSettings.mode = time_mode;
          }
        }
        if (time["execution"]) {
          commonSettings.secondsForEachIteration =
              time["execution"].as<std::size_t>();
        }
      }
      if (common["init-nodes"] && common["init-nodes"].IsSequence()) {
        std::cout << "in if\n";
        std::size_t total_iter = 0;
        for (const auto &node : common["init-nodes"]) {
          commonSettings.numInitialActiveNodes.push_back(
              node.as<std::size_t>());
          ++total_iter;
        }
        if (!total_iter) {
          throw std::runtime_error("init-nodes must have at least one value");
        }
      } else if (!is_benchmark_override) {
        throw std::runtime_error(
            "init-nodes must be specified mandatorily in the yaml file");
      }
      if (common["hash-functions"] && common["hash-functions"].IsSequence()) {
        for (const auto &hash_function : common["hash-functions"]) {
          if (hash_function.IsScalar()) {
            commonSettings.hashFunctions.push_back(
                hash_function.as<std::string>());
          }
        }
      }
      if (common["key-distributions"] &&
          common["key-distributions"].IsSequence()) {
        std::size_t total_iter = 0;
        for (const auto &keyDistribution : common["key-distributions"]) {
          commonSettings.keyDistributions.push_back(
              keyDistribution.as<std::string>());
          ++total_iter;
        }
        if (!total_iter) {
          throw std::invalid_argument("key-distributions must have at least "
                                      "one value specified in the yaml file");
        }
      } else if (!is_benchmark_override) {
        throw std::invalid_argument("no key-distribution key found in the yaml "
                                    "file, which is mandatory");
      }
    }
  }

  const std::vector<AlgorithmSettings> &getAlgorithms() const noexcept {
    return m_algorithms;
  }

  const CommonSettings &getCommonSettings() const noexcept {
    return m_commonSettings;
  }

  const std::vector<BenchmarkSettings> &getBenchmarks() const noexcept {
    return m_benchmarks;
  }

  // Just for debug, remove later
  void print() const {
    std::cout << "m_outputFolder: " << m_commonSettings.outputFolder
              << std::endl;
    std::cout << "m_totalBenchmarkIterations: "
              << m_commonSettings.totalBenchmarkIterations << std::endl;
    std::cout << "m_unit: " << m_commonSettings.unit << std::endl;
    std::cout << "m_mode: " << m_commonSettings.mode << std::endl;
    std::cout << "m_secondsForEachIteration: "
              << m_commonSettings.secondsForEachIteration << std::endl;

    std::cout << "m_numInitialActiveNodes: ";
    for (const auto &num : m_commonSettings.numInitialActiveNodes) {
      std::cout << num << " ";
    }
    std::cout << std::endl;

    std::cout << "m_hashFunctions: ";
    for (const auto &hashFunction : m_commonSettings.hashFunctions) {
      std::cout << hashFunction << " ";
    }
    std::cout << std::endl;

    std::cout << "m_keyDistributions: ";
    for (const auto &keyDistribution : m_commonSettings.keyDistributions) {
      std::cout << keyDistribution << " ";
    }
    std::cout << std::endl;

    std::cout << "Algorithms:" << std::endl;
    for (const auto &algorithm : m_algorithms) {
      std::cout << "Algorithm name: " << algorithm.name << std::endl;
      std::cout << "Arguments:" << std::endl;
      for (const auto &arg : algorithm.args) {
        std::cout << arg.first << ": " << arg.second << std::endl;
      }
      std::cout << std::endl;
    }

    std::cout << "Benchmarks:" << std::endl;
    for (const auto &benchmark : m_benchmarks) {
      std::cout << "Benchmark name: " << benchmark.name << std::endl;
      std::cout << "Common Settings:" << std::endl;
      std::cout << "m_outputFolder: " << benchmark.commonSettings.outputFolder
                << std::endl;
      std::cout << "m_totalBenchmarkIterations: "
                << benchmark.commonSettings.totalBenchmarkIterations
                << std::endl;
      std::cout << "m_unit: " << benchmark.commonSettings.unit << std::endl;
      std::cout << "m_mode: " << benchmark.commonSettings.mode << std::endl;
      std::cout << "m_secondsForEachIteration: "
                << benchmark.commonSettings.secondsForEachIteration
                << std::endl;

      std::cout << "m_numInitialActiveNodes: ";
      for (const auto &num : benchmark.commonSettings.numInitialActiveNodes) {
        std::cout << num << " ";
      }
      std::cout << std::endl;

      std::cout << "m_hashFunctions: ";
      for (const auto &hashFunction : benchmark.commonSettings.hashFunctions) {
        std::cout << hashFunction << " ";
      }
      std::cout << std::endl;

      std::cout << "m_keyDistributions: ";
      for (const auto &keyDistribution :
           benchmark.commonSettings.keyDistributions) {
        std::cout << keyDistribution << " ";
      }
      std::cout << std::endl;

      std::cout << "Arguments:" << std::endl;
      for (const auto &arg : benchmark.args) {
        std::cout << arg.first << ": " << arg.second << std::endl;
      }
      std::cout << std::endl;
    }
  }
};

#endif
