#ifndef CSVWRITER_H
#define CSVWRITER_H
#include <vector>
#include <string>
#include <fstream>
#include <type_traits>
#include <filesystem>
#include <typeinfo>
#include <functional>
#include "csv_structures.h"


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

	template<typename U = T, typename std::enable_if<std::is_same<U, ResizeTime>::value || std::is_same<U, InitTime>::value>::type* = nullptr>
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

	template<typename U = T, typename std::enable_if<std::is_same<U, ResizeTime>::value || std::is_same<U, InitTime>::value>::type* = nullptr>
	void write(const std::filesystem::path& directory) {
		if constexpr (std::is_same<U, ResizeTime>::value) {
			m_file_path = directory / "ResizeTime.csv";
		}
		else {
			m_file_path = directory / "InitTime.csv";
		}
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


#endif