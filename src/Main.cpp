#include "Common.hpp"
#include "EXPRESS.hpp"

#include "Platform.hpp"
#include "AP242.hpp"

extern "C" void start() {

	Write_String arg_line = { g_scratch_buffer, g_scratch_buffer_size };
	get_command_line(arg_line);
	g_scratch_buffer += arg_line.size;
	
	Read_String file = get_file_from_command_line({ arg_line.data, arg_line.size });

	if (is_file(file)) {
		print(fromcstr<Read_String>("Opening file: "));
		print(file);
		print(fromcstr<Read_String>("\n"));

		Write_String file_contents;
		file_contents.size = get_file_size({ file.data, file.size });
		file_contents.data = alloc<u8>(file_contents.size);
		defer {
			free(file_contents.data);
		};
		auto result = read_entire_file_into(file, file_contents);
		if (result != read_entire_file_into_result::success) {
			print(fromcstr<Read_String>("\nFailed to open file\n"));
			return;
		}

		double t1 = monotonic_seconds();
		auto res = parse_express_from_memory({ file_contents.data, file_contents.size });
		if (res.error) {
			print(fromcstr<Read_String>("\nError parsing file\n"));
			for (size_t i = 0; i < res.diagnostic.size; ++i) {
				print(res.diagnostic[i]);
				print("\n");
			}
			return;
		} else {
			print("\nParsed file\n");
			A242 a242;
			compile_express_from_memory(res, a242);
			double t2 = monotonic_seconds();
			print("Time: ");
			print((size_t)((t2 - t1) * 1000));
			print("ms\n");

		}
	}
	else if (is_directory(file)) {
		print(fromcstr<Read_String>("Opening directory: "));
		print(file);
		print(fromcstr<Read_String>("\n"));
		DynArray<Owned_String> files;
		recursive_list_files_in_directory(file, files);

		DynArray<size_t> failed;
		parse_express_from_memory_result res;
		A242 a242;
		
		double big_t1 = monotonic_seconds();

		for (size_t i = 0; i < files.size; ++i) {
			print("  ");
			print({ files[i].data, files[i].size });
			print("\n");

			auto_release_scratch();
			double t1 = monotonic_seconds();
			Write_String file_contents;
			file_contents.size = get_file_size({ files[i].data, files[i].size });
			// if (file_contents.size > 1024 * 1024)
			//	continue;
			file_contents.data = alloc<u8>(file_contents.size);
			defer {
				free(file_contents.data);
			};

			auto result = read_entire_file_into({ files[i].data, files[i].size }, file_contents);
			g_scratch_buffer += file_contents.size;
			if (result != read_entire_file_into_result::success) {
				print(fromcstr<Read_String>("Failed to open file\n"));
				failed.push(i);
				continue;
			}

			res.clear();
			parse_express_from_memory({ file_contents.data, file_contents.size }, res);
			if (res.error) {
				print(fromcstr<Read_String>("Error parsing file\n"));
				for (size_t i = 0; i < res.diagnostic.size; ++i) {
					print(res.diagnostic[i]);
					print("\n");
				}
				failed.push(i);
			}

			a242.clear();
			compile_express_from_memory(res, a242);
			double t2 = monotonic_seconds();
			print("    Time: ");
			print((size_t)((t2 - t1) * 1000));
			print("ms\n");
			print("\n");
		}

		if (failed.size > 0) {
			print("\nFailed files:\n");
			for (size_t i = 0; i < failed.size; i += 1) {
				print({ files[failed[i]].data, files[failed[i]].size });
				print("\n");
			}
		}
		double big_t2 = monotonic_seconds();
		print("Total time: ");
		print((size_t)((big_t2 - big_t1) * 1000));
		print("ms\n");
		print("\n");
		print("Time per file: ");
		print((size_t)(((big_t2 - big_t1) / files.size) * 1000));
		print("ms\n");
	}
	else {
		print(fromcstr<Read_String>("File or directory not found: "));
		print(file);
		print(fromcstr<Read_String>("\n"));
	}


}
