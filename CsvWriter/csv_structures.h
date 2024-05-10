#ifndef CSV_STRUCTS_H
#define CSV_STRUCTS_H

#include <string>
#include <cstddef>

struct MemoryUsage {
	std::string type;
	std::string algorithm{};
	std::size_t allocations{};
	std::size_t deallocations{};
	std::size_t allocated{};
	std::size_t deallocated{};
	std::size_t nodes{};
	std::string hash_function{};
	std::size_t iterations{};
	std::size_t maximum{};
};

struct Monotonicity {
	std::string hash_function{};
	std::string algorithm_name{};
	double fraction{};
	std::size_t keys{};
	std::string distribution{};
	std::size_t nodes{};
	std::size_t keys_in_removed_nodes{};
	std::size_t keys_moved_from_removed_nodes{};
	std::size_t keys_moved_from_other_nodes{};
	double nodes_losing_keys{};
	std::size_t keys_moved_to_restored_nodes{};
	std::size_t keys_moved_to_other_nodes{};
	double nodes_gaining_keys{};
	std::size_t keys_relocated_after_resize{};
	std::size_t nodes_changed_after_resize{};
	double keys_moved_from_removed_nodes_percentage{};
	double keys_moved_from_other_nodes_percentage{};
	double nodes_losing_keys_percentage{};
	double keys_moved_to_restored_nodes_percentage{};
	double keys_moved_to_other_nodes_percentage{};
	double nodes_gaining_keys_percentage{};
	double keys_relocated_after_resize_percentage{};
	double nodes_changed_after_resize_percentage{};

	// constructor for initialization of values found in YAML file
	explicit Monotonicity(const std::string& hash, const std::string& algo,
		double fraction, std::size_t keys, const std::string distribution,
		std::size_t nodes)
		: hash_function{ hash }, algorithm_name{ algo }, fraction{ fraction }
		, keys{ keys }, distribution{ distribution }, nodes{ nodes }
	{
	}
};

struct Balance {
	std::string hash_function{};
	std::string algorithm_name{};
	std::size_t keys{};
	std::string distribution{};
	std::size_t nodes{};
	std::size_t iterations{};
	double min{};
	double max{};
	std::size_t expected{};
	double min_percentage{};
	double max_percentage{};

	// constructor for initialization of values found in YAML file
	explicit Balance(const std::string& hash, const std::string& algo,
		std::size_t keys, const std::string& distribution, std::size_t nodes,
		std::size_t iterations)
		: hash_function{ hash }, algorithm_name{ algo }, keys{ keys }
		, distribution{ distribution }, nodes{ nodes }, iterations{ iterations }
	{
	}
};

struct LookupTime {
	std::string benchmark{};
	std::string mode{};
	std::size_t threads{};
	std::size_t samples{};
	double score{};
	double score_error{};
	std::string unit{};
	std::string param_algorithm{};
	std::string param_benchmark{};
	std::string param_distribution{};
	std::string param_function{};
	std::size_t param_init_nodes{};

	explicit LookupTime(const std::string& benchmark, const std::string& mode,
		std::size_t threads, std::size_t samples, const std::string& unit,
		const std::string& param_algorithm, const std::string& param_benchmark,
		const std::string& param_distribution, const std::string& param_function,
		std::size_t param_init_nodes)
		: benchmark(benchmark), mode(mode), threads(threads), samples(samples)
		, unit(unit), param_algorithm(param_algorithm)
		, param_benchmark(param_benchmark), param_distribution(param_distribution)
		, param_function(param_function), param_init_nodes(param_init_nodes)
	{
	}
};

struct CommonBase {
	std::string benchmark{};
	std::string mode{};
	std::size_t threads{};
	std::size_t samples{};
	double score{};
	double score_error{};
	std::string unit{};
	std::string param_algorithm{};
	std::string param_function{};
	std::size_t param_init_nodes{};

	explicit CommonBase(const std::string& benchmark, const std::string& mode,
		std::size_t threads, std::size_t samples, const std::string& unit,
		const std::string& param_algorithm, const std::string& param_function,
		std::size_t param_init_nodes)
		: benchmark(benchmark), mode(mode), threads(threads), samples(samples)
		, unit(unit), param_algorithm(param_algorithm)
		, param_function(param_function), param_init_nodes(param_init_nodes)
	{
	}
};

// Just for name clarity
struct ResizeTime : CommonBase {
	explicit ResizeTime(const std::string& benchmark, const std::string& mode,
		std::size_t threads, std::size_t samples, const std::string& unit,
		const std::string& param_algorithm, const std::string& param_function,
		std::size_t param_init_nodes)
		: CommonBase(benchmark, mode, threads, samples, unit,
			param_algorithm, param_function, param_init_nodes)
	{
	}
};

// Just for name clarity
struct InitTime : CommonBase {
	explicit InitTime(const std::string& benchmark, const std::string& mode,
		std::size_t threads, std::size_t samples, const std::string& unit,
		const std::string& param_algorithm, const std::string& param_function,
		std::size_t param_init_nodes)
		: CommonBase(benchmark, mode, threads, samples, unit,
			param_algorithm, param_function, param_init_nodes)
	{
	}
};


#endif