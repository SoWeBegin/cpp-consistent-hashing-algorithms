
#include "YamlParser/YamlParser.h"
#include <filesystem>
#include <fstream>
#include "metrics/monotonicity.h"
#include "metrics/balance.h"
#include "metrics/lookup_time.h"
#include "metrics/resize_time.h"
#include "metrics/init_time.h"
#include "CsvWriter/csv_writer_handler.h"
#include "utils.h"
#include "unordered_map"


int main(int argc, char* argv[]) {

    YamlParser parser("", "template.yaml");
    const auto& algorithms = parser.getAlgorithms();
    const auto& benchmarks = parser.getBenchmarks();
    const auto& commonSettings = parser.getCommonSettings();
   
    std::unordered_map<std::string, random_distribution_ptr<uint64_t>> distribution_function;
    distribution_function["uniform"] = &random_uniform_distribution<uint64_t>;

    CsvWriterHandler<Balance, Monotonicity, LookupTime, MemoryUsage, ResizeTime, InitTime> csv_writer_handler;

    for (const auto& current_benchmark : benchmarks) { // Done for all benchmarks in Java
        if (current_benchmark.name == "monotonicity") {
            monotonicity(csv_writer_handler.get_writer<Monotonicity>(), 
                commonSettings.outputFolder, current_benchmark, algorithms,
                distribution_function);
        }
        else if (current_benchmark.name == "balance") {
             balance(csv_writer_handler.get_writer<Balance>(), 
                 commonSettings.outputFolder, current_benchmark, algorithms,
                 commonSettings.totalBenchmarkIterations, distribution_function);
        }
        else if (current_benchmark.name == "lookup-time") {
            csv_writer_handler.update_get_writer_called<MemoryUsage>(); // LookupTime also does MemoryUsage bench
             speed_test(csv_writer_handler.get_writer<LookupTime>(), 
                 commonSettings.outputFolder, current_benchmark, algorithms,
                commonSettings, distribution_function);
        }
        else if (current_benchmark.name == "resize-time") {
            resize_time(csv_writer_handler.get_writer<ResizeTime>(),
                commonSettings.outputFolder, current_benchmark, algorithms,
                commonSettings);
        }
        else if (current_benchmark.name == "init-time") {
            init_time(csv_writer_handler.get_writer<InitTime>(),
                commonSettings.outputFolder, current_benchmark, algorithms,
                commonSettings);
        }
    }

    csv_writer_handler.write_all("./");
}