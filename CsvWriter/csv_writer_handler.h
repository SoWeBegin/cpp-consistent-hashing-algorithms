#ifndef CSV_WRITER_HANDLER_H
#define CSV_WRITER_HANDLER_H

#include "csvWriter.h"

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