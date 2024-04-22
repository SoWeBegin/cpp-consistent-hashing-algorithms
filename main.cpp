
#include "YamlParser/YamlParser.h"
#include <filesystem>
#include <fstream>
#include "monotonicity-v2.h"

int main(int argc, char* argv[]) {
    YamlParser parser("C:\\Users\\lucia\\Desktop\\", "template.yaml");
    parser.print();
    const auto& algorithms = parser.getAlgorithms();
    const auto& benchmarks = parser.getBenchmarks();
    const auto& commonSettings = parser.getCommonSettings();

    for (const auto& current_benchmark : benchmarks) {
        for (const auto& current_working_set : current_benchmark.commonSettings.numInitialActiveNodes) {
            for (const auto& current_hash_function : current_benchmark.commonSettings.hashFunctions) {
                for (const auto& current_key_distribution : current_benchmark.commonSettings.keyDistributions) {
                    if (current_benchmark.name == "monotonicity") {
                        monotonicity(commonSettings.outputFolder, current_working_set, current_hash_function,
                            current_key_distribution, algorithms, current_benchmark.args);
                    }
                }
            }
        }
    }


    // TODO:
    // For each specified benchmark, call the corresponding bench function. Do that for each specified algorithm
    
}