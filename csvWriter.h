#ifndef CSVWRITER_H
#define CSVWRITER_H
#include <unordered_map>
#include <string>


struct Monotonicity {
	std::string hash_function;
	std::string algorithm_name;
	double fraction;
	std::size_t keys;
	std::string distribution;
	std::size_t nodes;
	std::size_t keys_in_removed_nodes;
	std::size_t keys_moved_from_removed_nodes;
	std::size_t keys_moved_from_other_nodes;
	std::size_t nodes_losing_keys;
	std::size_t keys_moved_to_restored_nodes;
	std::size_t keys_moved_to_other_nodes;
	std::size_t nodes_gaining_keys;
	std::size_t keys_relocated_after_resize;
	std::size_t nodes_changed_after_resize;
	std::size_t keys_moved_from_removed_nodes_percentage;
	std::size_t keys_moved_from_other_nodes_percentage;
	std::size_t nodes_losing_keys_percentage;
	std::size_t keys_moved_to_restored_nodes_percentage;
	std::size_t keys_moved_to_other_nodes_percentage;
	std::size_t nodes_gaining_keys_percentage;
	std::size_t keys_relocated_after_resize_percentage;
	std::size_t nodes_changed_after_resize_percentage;
};

template<typename T>
class CsvWriter {
private: 
	T m_cache;


public:


};


#endif