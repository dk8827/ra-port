#include "mac_sdl.h"

#include <string>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <windows.h>

int PASCAL WinMain(HINSTANCE instance, HINSTANCE previous, char *command_line, int command_show);

static bool path_exists(char const *path)
{
	struct stat info;
	return path && stat(path, &info) == 0;
}

static std::string parent_path(std::string path)
{
	if (path.empty()) {
		return ".";
	}
	while (path.size() > 1 && path[path.size() - 1] == '/') {
		path.erase(path.size() - 1);
	}
	std::string::size_type slash = path.find_last_of('/');
	if (slash == std::string::npos) {
		return ".";
	}
	if (slash == 0) {
		return "/";
	}
	return path.substr(0, slash);
}

static std::string join_path(std::string const &left, char const *right)
{
	if (left.empty() || left == ".") {
		return right ? right : "";
	}
	if (left[left.size() - 1] == '/') {
		return left + (right ? right : "");
	}
	return left + "/" + (right ? right : "");
}

static bool use_resource_root(std::string const &root)
{
	static char const *config_path = "assets/redalert/allies/INSTALL/REDALERT.INI";
	if (root.empty()) {
		return false;
	}
	if (!path_exists(join_path(root, config_path).c_str())) {
		return false;
	}
	return chdir(root.c_str()) == 0;
}

static void select_resource_root(void)
{
	static char const *config_path = "assets/redalert/allies/INSTALL/REDALERT.INI";
	if (path_exists(config_path)) {
		return;
	}

	char *base_path = SDL_GetBasePath();
	if (!base_path) {
		return;
	}
	std::string executable_dir = base_path;
	SDL_free(base_path);

	std::string build_parent = parent_path(executable_dir);

	if (use_resource_root(executable_dir)) {
		return;
	}
	if (path_exists(config_path)) {
		return;
	}
	use_resource_root(build_parent);
}

int main(int argc, char **argv)
{
	SDL_SetMainReady();
	select_resource_root();

	std::string command_line;
	for (int index = 1; index < argc; ++index) {
		if (!command_line.empty()) {
			command_line += ' ';
		}
		command_line += argv[index];
	}
	return WinMain((HINSTANCE)0, (HINSTANCE)0, command_line.empty() ? (char *)"" : (char *)command_line.c_str(), SW_RESTORE);
}
