#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "yaml-cpp/parser.h"
#include "yaml-cpp/yaml.h"

#include <iostream>

struct AlgorithmSettings final {
	std::string name;
	std::unordered_map<std::string, std::string> args; // [argumentName][argumentValue]
};

struct CommonSettings final {
	std::string outputFolder;
	std::size_t totalBenchmarkIterations;
	std::string unit;
	std::string mode;
	std::size_t secondsForEachIteration;
	std::vector<std::size_t> numInitialActiveNodes;
	std::vector<std::string> hashFunctions;
	std::vector<std::string> keyDistributions;
};

struct BenchmarkSettings final {
	std::string name;
	CommonSettings commonSettings;
	std::unordered_map<std::string, std::string> args; // [argumentName][argumentValue] 
};


class YamlParser final {
private:
	CommonSettings m_commonSettings;
	std::vector<AlgorithmSettings> m_algorithms;
	std::vector<BenchmarkSettings> m_benchmarks;

public:
	explicit YamlParser(const std::filesystem::path& directory, const std::string& fileName) {
		const std::filesystem::path filePath = "template.yaml";//directory / fileName;

		if (!std::filesystem::exists(filePath)) {
			std::cerr << "[YamlParser::YamlParser]: Yaml file (" << filePath << ") not found!\n";
			return;
		}

		std::ifstream file(filePath);
		if (!file.is_open()) {
			std::cerr << "[YamlParser::YamlParser]: Unable to open file (" << filePath << ")!\n";
			return;
		}

		const auto m_config = YAML::LoadFile(filePath.string());
		if (m_config) {
			parseCommon(m_config["common"], m_commonSettings);
			parseAlgorithms(m_config["algorithms"]);
			parseBenchmarks(m_config["benchmarks"]);
		}
		else {
			std::cerr << "[YamlParser::YamlParser]: Error loading Yaml file (" << filePath << ")!\n";
		}
	}

	void parseBenchmarks(const YAML::Node& benchmarks) {
		if (benchmarks) {
			for (const auto& benchmark : benchmarks) {
				if (benchmark["name"]) {
					BenchmarkSettings benchmarkSettings{ benchmark["name"].as<std::string>(), m_commonSettings };

					if (benchmark["common"]) {
						parseCommon(benchmark["common"], benchmarkSettings.commonSettings);
					}

					if (benchmark["args"]) {
						for (const auto& pair : benchmark["args"]) {
							const YAML::Node& key = pair.first;
							const YAML::Node& value = pair.second;

							if (key.IsScalar()) {
								const std::string argName = key.as<std::string>();
								std::string argValue;

								if (value.IsScalar()) {
									argValue = value.as<std::string>();
								}
								else { // otherwise .as<std::string> throws a bad conversion exception
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

	void parseAlgorithms(const YAML::Node& algorithms) {
		if (algorithms) {
			for (const auto& algorithm : algorithms) {
				if (algorithm["name"]) {
					AlgorithmSettings algorithmSettings(algorithm["name"].as<std::string>());

					if (algorithm["args"]) {
						for (const auto& pair : algorithm["args"]) { // [key][value]
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

	void parseCommon(const YAML::Node& common, CommonSettings& commonSettings) {
		if (common) {
			if (common["output-folder"]) {
				commonSettings.outputFolder = common["output-folder"].as<std::string>();
			}
			if (common["iterations"]) {
				auto& iterations = common["iterations"];
				if (iterations["execution"]) {
					commonSettings.totalBenchmarkIterations = iterations["execution"].as<std::size_t>();
				}
			}
			if (common["time"]) {
				auto& time = common["time"];
				if (time["unit"]) {
					commonSettings.unit = time["unit"].as<std::string>();
					if (time["mode"]) {
						commonSettings.mode = time["mode"].as<std::string>();
						if (time["execution"]) {
							commonSettings.secondsForEachIteration = time["execution"].as<std::size_t>();
						}
					}
				}
			}
			if (common["init-nodes"] && common["init-nodes"].IsSequence()) {
				for (const auto& node : common["init-nodes"]) {
					commonSettings.numInitialActiveNodes.push_back(node.as<std::size_t>());
				}
			}
			if (common["hash-functions"] && common["hash-functions"].IsSequence()) {
				for (const auto& hash_function : common["hash-functions"]) {
					if (hash_function.IsScalar()) {
						commonSettings.hashFunctions.push_back(hash_function.as<std::string>());
					}
				}
			}
			if (common["key-distributions"] && common["key-distributions"].IsSequence()) {
				for (const auto& keyDistribution : common["key-distributions"]) {
					if (keyDistribution.IsScalar()) {
						commonSettings.keyDistributions.push_back(keyDistribution.as<std::string>());
					}
				}
			}
		}
	}

	// Just for debug, remove later
	void print() const {
		std::cout << "m_outputFolder: " << m_commonSettings.outputFolder << std::endl;
		std::cout << "m_totalBenchmarkIterations: " << m_commonSettings.totalBenchmarkIterations << std::endl;
		std::cout << "m_unit: " << m_commonSettings.unit << std::endl;
		std::cout << "m_mode: " << m_commonSettings.mode << std::endl;
		std::cout << "m_secondsForEachIteration: " << m_commonSettings.secondsForEachIteration << std::endl;

		std::cout << "m_numInitialActiveNodes: ";
		for (const auto& num : m_commonSettings.numInitialActiveNodes) {
			std::cout << num << " ";
		}
		std::cout << std::endl;

		std::cout << "m_hashFunctions: ";
		for (const auto& hashFunction : m_commonSettings.hashFunctions) {
			std::cout << hashFunction << " ";
		}
		std::cout << std::endl;

		std::cout << "m_keyDistributions: ";
		for (const auto& keyDistribution : m_commonSettings.keyDistributions) {
			std::cout << keyDistribution << " ";
		}
		std::cout << std::endl;

		std::cout << "Algorithms:" << std::endl;
		for (const auto& algorithm : m_algorithms) {
			std::cout << "Algorithm name: " << algorithm.name << std::endl;
			std::cout << "Arguments:" << std::endl;
			for (const auto& arg : algorithm.args) {
				std::cout << arg.first << ": " << arg.second << std::endl;
			}
			std::cout << std::endl;
		}

		std::cout << "Benchmarks:" << std::endl;
		for (const auto& benchmark : m_benchmarks) {
			std::cout << "Benchmark name: " << benchmark.name << std::endl;
			std::cout << "Common Settings:" << std::endl;
			std::cout << "m_outputFolder: " << benchmark.commonSettings.outputFolder << std::endl;
			std::cout << "m_totalBenchmarkIterations: " << benchmark.commonSettings.totalBenchmarkIterations << std::endl;
			std::cout << "m_unit: " << benchmark.commonSettings.unit << std::endl;
			std::cout << "m_mode: " << benchmark.commonSettings.mode << std::endl;
			std::cout << "m_secondsForEachIteration: " << benchmark.commonSettings.secondsForEachIteration << std::endl;

			std::cout << "m_numInitialActiveNodes: ";
			for (const auto& num : benchmark.commonSettings.numInitialActiveNodes) {
				std::cout << num << " ";
			}
			std::cout << std::endl;

			std::cout << "m_hashFunctions: ";
			for (const auto& hashFunction : benchmark.commonSettings.hashFunctions) {
				std::cout << hashFunction << " ";
			}
			std::cout << std::endl;

			std::cout << "m_keyDistributions: ";
			for (const auto& keyDistribution : benchmark.commonSettings.keyDistributions) {
				std::cout << keyDistribution << " ";
			}
			std::cout << std::endl;

			std::cout << "Arguments:" << std::endl;
			for (const auto& arg : benchmark.args) {
				std::cout << arg.first << ": " << arg.second << std::endl;
			}
			std::cout << std::endl;
		}
	}

};

#endif