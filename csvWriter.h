#ifndef CSVWRITER_H
#define CSVWRITER_H
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <type_traits>

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
};

template<typename T>
class CsvWriter {
private:
	std::vector<T> m_cache;
	std::ofstream output_file;
	std::filesystem::path m_file_path;

	explicit CsvWriter(const std::filesystem::path& directory, const std::string& file_name)
		: m_file_path{ directory / file_name }
	{
	}

public:
	static CsvWriter<T>& getInstance(const std::filesystem::path& directory, const std::string& file_name) {
		static CsvWriter<T> instance(directory, file_name);
		return instance;
	}

private:
	template<typename U = T, typename std::enable_if<std::is_same<U, Monotonicity>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "hash_function,"
			<< "algorithm_name,"
			<< "fraction,"
			<< "keys,"
			<< "distribution,"
			<< "nodes,"
			<< "keys_in_removed_nodes,"
			<< "keys_moved_from_removed_nodes,"
			<< "keys_moved_from_other_nodes,"
			<< "nodes_losing_keys,"
			<< "keys_moved_to_restored_nodes,"
			<< "keys_moved_to_other_nodes,"
			<< "nodes_gaining_keys,"
			<< "keys_relocated_after_resize,"
			<< "nodes_changed_after_resize,"
			<< "keys_moved_from_removed_nodes_percentage,"
			<< "keys_moved_from_other_nodes_percentage,"
			<< "nodes_losing_keys_percentage,"
			<< "keys_moved_to_restored_nodes_percentage,"
			<< "keys_moved_to_other_nodes_percentage,"
			<< "nodes_gaining_keys_percentage,"
			<< "keys_relocated_after_resize_percentage,"
			<< "nodes_changed_after_resize_percentage"
			<< '\n';
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, LookupTime>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "benchmark,"
			<< "mode,"
			<< "threads,"
			<< "samples,"
			<< "score,"
			<< "score_error,"
			<< "unit,"
			<< "param_algorithm,"
			<< "param_benchmark,"
			<< "param_distribution,"
			<< "param_function,"
			<< "param_init_nodes"
			<< "\n";
	}

	template<typename U = T, typename std::enable_if<std::is_same<U, Balance>::value>::type* = nullptr>
	void writeHeader() {
		output_file << "hash_function,"
			<< "algorithm_name,"
			<< "keys,"
			<< "distribution,"
			<< "nodes,"
			<< "iterations,"
			<< "min,"
			<< "max,"
			<< "expected,"
			<< "min_percentage,"
			<< "max_percentage"
			<< "\n";
	}

public:
	template<typename U = T, typename std::enable_if<std::is_same<U, Monotonicity>::value>::type* = nullptr>
	void write() {
		if (!output_file.is_open()) {
			output_file.open(m_file_path);
			writeHeader<T>();
		}
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
	void write() {
		if (!output_file.is_open()) {
			output_file.open(m_file_path);
			writeHeader<T>();
		}
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

	template<typename U = T, typename std::enable_if<std::is_same<U, LookupTime>::value>::type* = nullptr>
	void write() {
		if (!output_file.is_open()) {
			output_file.open(m_file_path);
			writeHeader<T>();
		}
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

	constexpr void add(const T& t) {
		m_cache.push_back(t);
	}
};




#endif