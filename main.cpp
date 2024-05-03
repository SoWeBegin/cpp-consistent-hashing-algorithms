
#include "YamlParser/YamlParser.h"
#include <filesystem>
#include <fstream>
#include "monotonicity-v2.h"
#include "balance-v2.h"
#include "speed_test-v2.h"
#include "csvWriter.h"


int main(int argc, char* argv[]) {
    YamlParser parser("", "template.yaml");
    parser.print();
    const auto& algorithms = parser.getAlgorithms();
    const auto& benchmarks = parser.getBenchmarks();
    const auto& commonSettings = parser.getCommonSettings();

    for (const auto& current_benchmark : benchmarks) { // Done for all benchmarks in Java
        if (current_benchmark.name == "monotonicity") {
            monotonicity(commonSettings.outputFolder, current_benchmark, algorithms);
        }
        /*else if (current_benchmark.name == "balance") {
             balance(commonSettings.outputFolder, current_algorithm, current_working_set, current_hash_function,
                 current_key_distribution, current_benchmark.args, commonSettings.totalBenchmarkIterations);
        }
        else if (current_benchmark.name == "lookup-time") {
            speed_test(commonSettings.outputFolder, current_algorithm, current_working_set, current_hash_function,
                current_key_distribution, current_benchmark.args, current_benchmark.commonSettings);
        }*/
    }
      
    auto& balance_writer = CsvWriter<Balance>::getInstance("./", "balance.csv");
    balance_writer.write();
    auto& monotonicity_writer = CsvWriter<Monotonicity>::getInstance("./", "monotonicity.csv");
    monotonicity_writer.write();
    auto& lookuptime_writer = CsvWriter<LookupTime>::getInstance("./", "lookup_time.csv");
    lookuptime_writer.write();
    // TODO:
    // For each specified benchmark, call the corresponding bench function. Do that for each specified algorithm
    
}