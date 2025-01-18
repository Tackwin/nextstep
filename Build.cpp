#include "Ease.hpp"

EASE_WATCH_ME;

Build build(Flags flags) noexcept {
	flags.no_install_path = true;
	
	Build b = Build::get_default(flags);
	b.flags.no_default_lib = true;
	b.flags.entry_point = "start";
	b.flags.subsystem = Flags::Subsystem::Console;
	b.flags.disable_exceptions = true;
	b.flags.compile_native = true;

	b.name = "nextstep";
	// b.cli = Build::Cli::Gcc;

	b.add_header("src/");
	b.add_source_recursively("src/");
	b.add_define("WIN32_LEAN_AND_MEAN");
	b.add_define("NOMINMAX");

	b.add_library("kernel32.lib");
	

	return b;
}