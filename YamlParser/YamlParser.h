#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <string>
#include <filesystem>
#include "yaml-cpp/parser.h"
#include "yaml-cpp/yaml.h"



#include <iostream>

// put everything inside a struct
class YamlParser final {
private:
	YAML::Node m_config;
	std::string m_outputFolder;
	std::size_t m_totalBenchmarkIterations;
	std::string m_unit;
	std::string m_mode;
	std::string m_secondsForEachIteration;
	std::vector<std::size_t> m_numInitialActiveNodes;
	std::vector<std::string> m_hashFunctions;
	std::vector<std::string> m_keyDistributions;


public:
	explicit YamlParser(const std::filesystem::path& directory, const std::string& fileName) {
		const std::filesystem::path filePath = directory / fileName;
		std::cout << "File Path: " << filePath << std::endl; // Debug 
		m_config = YAML::LoadFile(filePath.string());
		if (m_config) {
			parseCommon(m_config["common"]);
		}
		else {
			std::cerr << "[YamlParser::YamlParser]: Yaml file (" << filePath << ") not found!\n";
		}
	}


	void parseCommon(const YAML::Node& common) {
		if (common) {
			if (common["output-folder"]) {
				m_outputFolder = common["output-folder"].as<std::string>();
			} else {
				std::cerr << "output-folder";
			}
			if (common["iterations"]) {
				auto& iterations = common["iterations"];
				if (iterations["execution"]) {
					m_totalBenchmarkIterations = iterations["executions"].as<std::size_t>();
				} else {
					std::cerr << "iterations";
				}
			} else {
				// error
			}
			if (common["time"]) {
				auto& time = common["time"];
				if (time["unit"]) {
					m_unit = time["unit"].as<std::string>();
				} else {
					std::cerr << "unit";
				}
				if (time["mode"]) {
					m_mode = time["mode"].as<std::string>();
				} else {
					std::cerr << "mode";
				}
				if (time["execution"]) {
					m_secondsForEachIteration = time["execution"].as<std::string>();
				} else {
					std::cerr << "execution";
				}
			}
			else {
				std::cerr << "time";
			}
			if (common["init-notes"] && common["init-nodes"].IsSequence()) {
				for (const auto& node : common["init-nodes"]) {
					m_numInitialActiveNodes.push_back(node.as<std::size_t>());
				}
			} else {
				std::cerr << "Error: 'init-nodes' key is missing or not a sequence." << std::endl;
			}
			if (common["hash-functions"] && common["hash-functions"].IsSequence()) {
				for (const auto& hash_function : common["hash-functions"]) {
					if (hash_function.IsScalar()) {
						m_hashFunctions.push_back(hash_function.as<std::string>());
					}
				}
			} else {
				std::cerr << "hash-functions";
			}
			if (common["key-distributions"] && common["key-distributions"].IsSequence()) {
				for (const auto& keyDistribution : common["key-distributions"]) {
					if (keyDistribution.IsScalar()) {
						m_keyDistributions.push_back(keyDistribution.as<std::string>());
					}
				}
			}
			else {
				std::cerr << "key-distribution";
			}
		} else {
			std::cerr << "common";
		}
	}

	void print() const {
		std::cout << "m_config: " << m_config << std::endl;
		std::cout << "m_outputFolder: " << m_outputFolder << std::endl;
		std::cout << "m_totalBenchmarkIterations: " << m_totalBenchmarkIterations << std::endl;
		std::cout << "m_unit: " << m_unit << std::endl;
		std::cout << "m_mode: " << m_mode << std::endl;
		std::cout << "m_secondsForEachIteration: " << m_secondsForEachIteration << std::endl;

		std::cout << "m_numInitialActiveNodes: ";
		for (const auto& num : m_numInitialActiveNodes) {
			std::cout << num << " ";
		}
		std::cout << std::endl;

		std::cout << "m_hashFunctions: ";
		for (const auto& hashFunction : m_hashFunctions) {
			std::cout << hashFunction << " ";
		}
		std::cout << std::endl;

		std::cout << "m_keyDistributions: ";
		for (const auto& keyDistribution : m_keyDistributions) {
			std::cout << keyDistribution << " ";
		}
		std::cout << std::endl;
	}
};

#endif