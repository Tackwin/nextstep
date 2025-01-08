#include "Common.hpp"
#include "EXPRESS.hpp"

#include "Platform.hpp"

extern "C" void start() {

	Write_String arg_line = { g_scratch_buffer, g_scratch_buffer_size };
	get_command_line(arg_line);
	g_scratch_buffer += arg_line.size;
	
	Read_String file = get_file_from_command_line({ arg_line.data, arg_line.size });

	print(fromcstr<Read_String>("Opening file: "));
	print(file);
	print(fromcstr<Read_String>("\n"));

	Write_String file_contents = { g_scratch_buffer, g_scratch_buffer_size };
	g_scratch_buffer += file_contents.size;
	auto result = read_entire_file_into(file, file_contents);
	if (result != read_entire_file_into_result::success) {
		print(fromcstr<Read_String>("\nFailed to open file\n"));
		return;
	}

	auto res = parse_express_from_memory({ file_contents.data, file_contents.size });
	if (res.error) {
		print(fromcstr<Read_String>("\nError parsing file\n"));
		print({ res.diagnostic.data, res.diagnostic.size });
		return;
	} else {
		print(fromcstr<Read_String>("\nParsed file\n"));
	}

}
