#include <SDL.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <windows.h>

int PASCAL WinMain(HINSTANCE instance, HINSTANCE previous, char *command_line, int command_show);

static char const *config_path(void)
{
	return "assets/redalert/allies/INSTALL/REDALERT.INI";
}

static bool path_exists(char const *path)
{
	struct stat info;
	return path && stat(path, &info) == 0;
}

static bool directory_exists(char const *path)
{
	struct stat info;
	return path && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

static std::string join_path(std::string const &left, char const *right)
{
	if (left.empty()) {
		return right ? std::string(right) : std::string();
	}
	if (!right || !right[0]) {
		return left;
	}
	if (left[left.size() - 1] == '/') {
		return left + right;
	}
	return left + "/" + right;
}

static std::string parent_path(std::string const &path)
{
	std::string::size_type slash = path.find_last_of('/');
	if (slash == std::string::npos) {
		return ".";
	}
	if (slash == 0) {
		return "/";
	}
	return path.substr(0, slash);
}

static bool make_directory(char const *path)
{
	if (!path || !path[0]) {
		return false;
	}
	if (mkdir(path, 0755) == 0) {
		return true;
	}
	return errno == EEXIST && directory_exists(path);
}

static bool make_directories(std::string const &path)
{
	if (path.empty()) {
		return false;
	}

	std::string current;
	std::string::size_type index = 0;
	if (path[0] == '/') {
		current = "/";
		index = 1;
	}

	while (index < path.size()) {
		std::string::size_type slash = path.find('/', index);
		std::string part = slash == std::string::npos ? path.substr(index) : path.substr(index, slash - index);
		if (!part.empty()) {
			if (!current.empty() && current[current.size() - 1] != '/') {
				current += "/";
			}
			current += part;
			if (!make_directory(current.c_str())) {
				return false;
			}
		}
		if (slash == std::string::npos) {
			break;
		}
		index = slash + 1;
	}

	return directory_exists(path.c_str());
}

static bool copy_file(char const *source, char const *dest)
{
	int in = open(source, O_RDONLY);
	if (in < 0) {
		return false;
	}

	if (!make_directories(parent_path(dest))) {
		close(in);
		return false;
	}

	std::string pattern = std::string(dest) + ".tmp.XXXXXX";
	std::vector<char> temporary(pattern.begin(), pattern.end());
	temporary.push_back(0);
	int out = mkstemp(&temporary[0]);
	if (out < 0) {
		close(in);
		return false;
	}

	char buffer[32768];
	bool ok = true;
	for (;;) {
		ssize_t bytes = read(in, buffer, sizeof(buffer));
		if (bytes == 0) {
			break;
		}
		if (bytes < 0) {
			ok = false;
			break;
		}
		char const *cursor = buffer;
		ssize_t remaining = bytes;
		while (remaining > 0) {
			ssize_t written = write(out, cursor, (size_t)remaining);
			if (written <= 0) {
				ok = false;
				remaining = 0;
				break;
			}
			cursor += written;
			remaining -= written;
		}
		if (!ok) {
			break;
		}
	}

	if (close(out) != 0) {
		ok = false;
	}
	close(in);
	if (ok) ok = rename(&temporary[0], dest) == 0;
	if (!ok) unlink(&temporary[0]);
	return ok;
}

/*
 * Files at least this large are game data that is only read (MIX archives,
 * movies, audio). They are linked to the copy inside the app bundle instead of
 * duplicated, which keeps the app from storing the game data twice. Smaller
 * files (INI files and the like) are copied so the game can change them.
 */
static off_t const link_threshold = 64 * 1024;

static bool link_file(char const *source, char const *dest)
{
	char current[PATH_MAX];
	ssize_t length = readlink(dest, current, sizeof(current) - 1);
	if (length >= 0) {
		current[length] = 0;
		if (strcmp(current, source) == 0) {
			return true;
		}
	}
	if (!make_directories(parent_path(dest))) {
		return false;
	}
	/* Replace only after the new link exists, preserving the old asset if
	 * creating the replacement fails. rename also replaces dangling links. */
	std::string pattern = std::string(dest) + ".tmp.XXXXXX";
	std::vector<char> temporary(pattern.begin(), pattern.end());
	temporary.push_back(0);
	int fd = mkstemp(&temporary[0]);
	if (fd < 0) return false;
	close(fd);
	unlink(&temporary[0]);
	bool ok = symlink(source, &temporary[0]) == 0 && rename(&temporary[0], dest) == 0;
	if (!ok) unlink(&temporary[0]);
	return ok;
}

static bool sync_directory_recursive(char const *source, char const *dest)
{
	DIR *dir = opendir(source);
	if (!dir) {
		return false;
	}
	if (!make_directories(dest)) {
		closedir(dir);
		return false;
	}

	bool ok = true;
	for (;;) {
		struct dirent *entry = readdir(dir);
		if (!entry) {
			break;
		}
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		std::string source_path = join_path(source, entry->d_name);
		std::string dest_path = join_path(dest, entry->d_name);
		struct stat info;
		if (lstat(source_path.c_str(), &info) != 0) {
			ok = false;
			break;
		}
		if (S_ISDIR(info.st_mode)) {
			if (!sync_directory_recursive(source_path.c_str(), dest_path.c_str())) {
				ok = false;
				break;
			}
		} else if (S_ISREG(info.st_mode)) {
			struct stat dest_info;
			if (info.st_size >= link_threshold) {
				if (!link_file(source_path.c_str(), dest_path.c_str())) {
					ok = false;
					break;
				}
			} else if (lstat(dest_path.c_str(), &dest_info) != 0 || S_ISLNK(dest_info.st_mode)) {
				/* Preserve existing writable files, but replace links to older
				 * bundles even when the new asset is now below the size cutoff. */
				if (!copy_file(source_path.c_str(), dest_path.c_str())) {
					ok = false;
					break;
				}
			}
		}
	}

	closedir(dir);
	return ok;
}

static bool use_resource_root(std::string const &root)
{
	if (root.empty() || chdir(root.c_str()) != 0) {
		return false;
	}
	return path_exists(config_path());
}

static bool sync_bundled_assets_to(std::string const &writable_root)
{
	char *base_path = SDL_GetBasePath();
	if (!base_path) {
		return false;
	}

	std::string source = join_path(base_path, "assets/redalert");
	SDL_free(base_path);
	if (!directory_exists(source.c_str())) {
		return false;
	}

	std::string dest = join_path(writable_root, "assets/redalert");
	if (!sync_directory_recursive(source.c_str(), dest.c_str())) {
		return false;
	}
	return path_exists(join_path(writable_root, config_path()).c_str());
}

static bool select_resource_root(void)
{
	char *pref_path = SDL_GetPrefPath("ra-port", "RedAlert");
	if (pref_path) {
		std::string writable_root = join_path(pref_path, "redalert-root");
		SDL_free(pref_path);
		make_directories(writable_root);
		/*
		 * Run on every launch: each app update installs the bundle at a new
		 * path, so the links to its large files must be refreshed.
		 */
		if (!sync_bundled_assets_to(writable_root)) {
			fprintf(stderr, "Unable to synchronize bundled Red Alert assets into %s\n", writable_root.c_str());
			return false;
		}
		if (use_resource_root(writable_root)) {
			return true;
		}
	}

	char *base_path = SDL_GetBasePath();
	if (base_path) {
		std::string bundle_root = base_path;
		SDL_free(base_path);
		return use_resource_root(bundle_root);
	}
	return false;
}

extern "C" int SDL_main(int argc, char **argv)
{
	if (!select_resource_root()) return 1;

	std::string command_line;
	for (int index = 1; index < argc; ++index) {
		if (!command_line.empty()) {
			command_line += ' ';
		}
		command_line += argv[index];
	}

	return WinMain((HINSTANCE)0, (HINSTANCE)0, command_line.empty() ? (char *)"" : (char *)command_line.c_str(), SW_RESTORE);
}
