#ifndef CSVWRITER_H
#define CSVWRITER_H
#include <vector>
#include <string>
#include <fstream>
#include <type_traits>
#include <filesystem>
#include <typeinfo>
#include <functional>


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

struct ResizeTime {
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

	explicit ResizeTime(const std::string& benchmark, const std::string& mode,
		std::size_t threads, std::size_t samples, const std::string& unit,
		const std::string& param_algorithm,const std::string& param_function,
		std::size_t param_init_nodes)
		: benchmark(benchmark), mode(mode), threads(threads), samples(samples)
		, unit(unit), param_algorithm(param_algorithm)
		, param_function(param_function), param_init_nodes(param_init_nodes)
	{
	}
};

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

template<typename T>
class CsvWriter {
private:
	std::vector<T> m_cache;
	std::ofstream output_file;
	std::filesystem::path m_file_path;

	CsvWriter() = default;

public:
	static CsvWriter<T>& getInstance() {
		static CsvWriter<T> instance;
		return instance;
	}

private:
	template<typename U = T, typename std::enable_if<std::is_same<U, Monotonicity>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "Hash Function,"
			<< "Algorithm,"
			<< "Fraction,"
			<< "Keys,"
			<< "Distribution,"
			<< "Initial Nodes,"
			<< "KeysInRemovedNodes,"
			<< "KeysMovedFromRemovedNodes,"
			<< "KeysMovedFromOtherNodes,"
			<< "NodesLosingKeys,"
			<< "KeysMovedToRestoredNodes,"
			<< "KeysMovedToOtherNodes,"
			<< "NodesGainingKeys,"
			<< "KeysRelocatedAfterResize,"
			<< "NodesChangedAfterResize,"
			<< "KeysMovedFromRemovedNodes%,"
			<< "KeysMovedFromOtherNodes%,"
			<< "NodesLosingKeys%,"
			<< "KeysMovedToRestoredNodes%,"
			<< "KeysMovedToOtherNodes%,"
			<< "NodesGainingKeys%,"
			<< "KeysRelocatedAfterResize%,"
			<< "NodesChangedAfterResize%"
			<< '\n';
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, LookupTime>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "Benchmark, Mode, Threads, Samples, Score, Score Error (stddev), Unit, Algorithm,"
			<< "Benchmark, Distribution, Hash Function, Initial Nodes\n";
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, ResizeTime>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "Benchmark, Mode, Threads, Samples, Score, Score Error (stddev), Unit, Algorithm,"
			<< "Hash Function, Initial Nodes\n";
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, MemoryUsage>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "Type, Allocations, Deallocations, Allocated, Deallocated, Maximum,"
			<< "Algorithm, Nodes, Hash Function, Total Iterations\n";
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, Balance>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "Hash Function, Algorithm, Keys, Distribution, InitialNodes, TotalIterations, Min,"
			<< "Max, Expected, Min%, Max%\n";
	}

public:
	template<typename U = T, typename std::enable_if<std::is_same<U, Monotonicity>::value>::type* = nullptr>
	void write(const std::filesystem::path& directory) {

		m_file_path = directory / "Monotonicity.csv";
		open_file_if_closed();

		for (const auto& t : m_cache) {
			output_file << t.hash_function << ','
				<< t.algorithm_name << ','
				<< t.fraction << ','
				<< t.keys << ','
				<< t.distribution << ','
				<< t.nodes << ','
				<< t.keys_in_removed_nodes << ','
				<< t.keys_moved_from_removed_nodes << ','
				<< t.keys_moved_from_other_nodes << ','
				<< t.nodes_losing_keys << ','
				<< t.keys_moved_to_restored_nodes << ','
				<< t.keys_moved_to_other_nodes << ','
				<< t.nodes_gaining_keys << ','
				<< t.keys_relocated_after_resize << ','
				<< t.nodes_changed_after_resize << ','
				<< t.keys_moved_from_removed_nodes_percentage << ','
				<< t.keys_moved_from_other_nodes_percentage << ','
				<< t.nodes_losing_keys_percentage << ','
				<< t.keys_moved_to_restored_nodes_percentage << ','
				<< t.keys_moved_to_other_nodes_percentage << ','
				<< t.nodes_gaining_keys_percentage << ','
				<< t.keys_relocated_after_resize_percentage << ','
				<< t.nodes_changed_after_resize_percentage << '\n';
		}
		m_cache.clear();
		output_file.close();
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, Balance>::value>::type* = nullptr>
	void write(const std::filesystem::path& directory) {
		m_file_path = directory / "Balance.csv";
		open_file_if_closed();

		for (const auto& t : m_cache) {
			output_file << t.hash_function << ','
				<< t.algorithm_name << ','
				<< t.keys << ','
				<< t.distribution << ','
				<< t.nodes << ','
				<< t.iterations << ','
				<< t.min << ','
				<< t.max << ','
				<< t.expected << ','
				<< t.min_percentage << ','
				<< t.max_percentage << '\n';
		}
		m_cache.clear();
		output_file.close();
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, MemoryUsage>::value>::type* = nullptr>
	void write(const std::filesystem::path& directory) {
		m_file_path = directory / "MemoryUsage.csv";
		open_file_if_closed();

		for (const auto& t : m_cache) {
			output_file << t.type << ','
				<< t.allocations << ','
				<< t.deallocations << ','
				<< t.allocated << ','
				<< t.deallocated << ','
				<< t.maximum << ','
				<< t.algorithm << ','
				<< t.nodes << ','
				<< t.hash_function << ','
				<< t.iterations
				<< '\n';
		}
		m_cache.clear();
		output_file.close();
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, LookupTime>::value>::type* = nullptr>
	void write(const std::filesystem::path& directory) {
		m_file_path = directory / "LookupTime.csv";
		open_file_if_closed();

		for (const auto& t : m_cache) {
			output_file << t.benchmark << ','
				<< t.mode << ','
				<< t.threads << ','
				<< t.samples << ','
				<< t.score << ','
				<< t.score_error << ','
				<< t.unit << ','
				<< t.param_algorithm << ','
				<< t.param_benchmark << ','
				<< t.param_distribution << ','
				<< t.param_function << ','
				<< t.param_init_nodes << "\n";
		}
		m_cache.clear();
		output_file.close();
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, ResizeTime>::value>::type* = nullptr>
	void write(const std::filesystem::path& directory) {
		m_file_path = directory / "ResizeTime.csv";
		open_file_if_closed();

		for (const auto& t : m_cache) {
			output_file << t.benchmark << ','
				<< t.mode << ','
				<< t.threads << ','
				<< t.samples << ','
				<< t.score << ','
				<< t.score_error << ','
				<< t.unit << ','
				<< t.param_algorithm << ','
				<< t.param_function << ','
				<< t.param_init_nodes << "\n";
		}
		m_cache.clear();
		output_file.close();
	}

	constexpr void add(const T& t) {
		m_cache.push_back(t);
	}

	private:
		void open_file_if_closed() {
			if (!output_file.is_open()) {
				output_file.open(m_file_path);
				writeHeader<T>();
			}
		}
};


template<typename... Ts>
class CsvWriterHandler {
private:
	bool get_writer_called[sizeof...(Ts)]{};

public:
	template<typename T>
	CsvWriter<T>& get_writer() {
		update_get_writer_called<T>();
		return CsvWriter<T>::getInstance();
	}

	void write_all(const std::filesystem::path& directory) {
		(write_if_tracked<Ts>(directory), ...);
	}

	template<typename T>
	void update_get_writer_called() {
		std::size_t index = find_type_index<T, Ts...>();
		if (index < sizeof...(Ts)) {
			get_writer_called[index] = true;
		}
	}

private:
	template<typename T>
	void write_if_tracked(const std::filesystem::path& directory) {
		std::size_t index = find_type_index<T, Ts...>();
		if (index < sizeof...(Ts)) {
			if (get_writer_called[index]) {
				get_writer<T>().write(directory);
			}
		}
	}

	template <typename T, typename... Us>
	std::size_t find_type_index() {
		std::size_t index = 0;
		// Short circuiting (note: ++ has higher precedence than !)
		(... or (std::is_same_v<T, Us> or !++index));
		return index;
	}
};




#endif