
#include "YamlParser/YamlParser.h"
#include <filesystem>
#include <fstream>
#include "monotonicity-v2.h"
#include "balance-v2.h"
#include "speed_test-v2.h"
#include "csvWriter.h"
#include "utils.h"
#include "unordered_map"


int main(int argc, char* argv[]) {
    YamlParser parser("", "template.yaml");
    parser.print();
    const auto& algorithms = parser.getAlgorithms();
    const auto& benchmarks = parser.getBenchmarks();
    const auto& commonSettings = parser.getCommonSettings();

    // Lazy initialization of the random functions:
    // First call is slower because the generator must be initialized, avoid that.
    random_uniform_distribution<uint32_t>();

    std::unordered_map<std::string, random_distribution_ptr<uint32_t>> distribution_function;
    distribution_function["uniform"] = &random_uniform_distribution<uint32_t>;

    for (const auto& current_benchmark : benchmarks) { // Done for all benchmarks in Java
        if (current_benchmark.name == "monotonicity") {
            monotonicity(commonSettings.outputFolder, current_benchmark, algorithms,
                distribution_function);
        }
        else if (current_benchmark.name == "balance") {
             balance(commonSettings.outputFolder, current_benchmark, algorithms,
                 commonSettings.totalBenchmarkIterations, distribution_function);
        }
        else if (current_benchmark.name == "lookup-time") {
             speed_test(commonSettings.outputFolder, current_benchmark, algorithms,
                commonSettings, distribution_function);
        }
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