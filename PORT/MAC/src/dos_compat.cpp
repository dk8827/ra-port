#include "dos.h"

#include <dirent.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>

#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif

struct FindState {
	std::string directory;
	std::vector<std::string> matches;
	size_t index;
};

static std::map<find_t *, FindState> FindStates;

static std::string dirname_of(char const *path)
{
	std::string value = path ? path : "";
	std::string::size_type slash = value.find_last_of("/\\");
	if (slash == std::string::npos) {
		return ".";
	}
	if (slash == 0) {
		return value.substr(0, 1);
	}
	return value.substr(0, slash);
}

static std::string basename_of(char const *path)
{
	std::string value = path ? path : "";
	std::string::size_type slash = value.find_last_of("/\\");
	if (slash == std::string::npos) {
		return value;
	}
	return value.substr(slash + 1);
}

static void fill_find_result(find_t *result, std::string const &directory, std::string const &name)
{
	struct stat st;
	std::string full = directory;
	if (!full.empty() && full != "/" && full[full.size() - 1] != '/') {
		full += "/";
	}
	full += name;

	memset(result, 0, sizeof(*result));
	strncpy(result->name, name.c_str(), sizeof(result->name) - 1);
	if (stat(full.c_str(), &st) == 0) {
		result->size = static_cast<unsigned long>(st.st_size);
		result->attrib = S_ISDIR(st.st_mode) ? _A_SUBDIR : _A_ARCH;
		if ((st.st_mode & S_IWUSR) == 0) {
			result->attrib |= _A_RDONLY;
		}
	}
}

extern "C" int _dos_findfirst(char const *path, unsigned, struct find_t *result)
{
	if (!path || !result) {
		errno = EINVAL;
		return -1;
	}

	std::string directory = dirname_of(path);
	std::string pattern = basename_of(path);
	DIR *dir = opendir(directory.c_str());
	if (!dir) {
		return -1;
	}

	FindState state;
	state.directory = directory;
	state.index = 0;
	for (dirent *entry = readdir(dir); entry; entry = readdir(dir)) {
		if (fnmatch(pattern.c_str(), entry->d_name, FNM_CASEFOLD) == 0) {
			state.matches.push_back(entry->d_name);
		}
	}
	closedir(dir);

	if (state.matches.empty()) {
		errno = ENOENT;
		return -1;
	}

	fill_find_result(result, directory, state.matches[0]);
	FindStates[result] = state;
	return 0;
}

extern "C" int _dos_findnext(struct find_t *result)
{
	std::map<find_t *, FindState>::iterator found = FindStates.find(result);
	if (found == FindStates.end()) {
		errno = ENOENT;
		return -1;
	}
	FindState &state = found->second;
	++state.index;
	if (state.index >= state.matches.size()) {
		errno = ENOENT;
		return -1;
	}
	fill_find_result(result, state.directory, state.matches[state.index]);
	return 0;
}

extern "C" int _dos_findclose(struct find_t *result)
{
	FindStates.erase(result);
	return 0;
}

extern "C" int _dos_getdrive(unsigned *drive)
{
	if (drive) {
		*drive = 3;
	}
	return 0;
}

extern "C" int _dos_setdrive(unsigned, unsigned *drives)
{
	if (drives) {
		*drives = 1;
	}
	return 0;
}

extern "C" int _dos_getdiskfree(unsigned, struct diskfree_t *diskspace)
{
	if (!diskspace) {
		errno = EINVAL;
		return -1;
	}
	struct statvfs info;
	if (statvfs(".", &info) != 0) {
		return -1;
	}
	diskspace->bytes_per_sector = static_cast<unsigned>(info.f_bsize);
	diskspace->sectors_per_cluster = 1;
	diskspace->total_clusters = static_cast<unsigned>(info.f_blocks);
	diskspace->avail_clusters = static_cast<unsigned>(info.f_bavail);
	return 0;
}

extern "C" int _dos_open(char const *path, unsigned mode, int *handle)
{
	int fd = open(path, mode, S_IREAD | S_IWRITE);
	if (handle) {
		*handle = fd;
	}
	return fd == -1 ? errno : 0;
}

extern "C" int _dos_close(int handle)
{
	return close(handle) == -1 ? errno : 0;
}

extern "C" unsigned _dos_read(int handle, void *buffer, unsigned count, unsigned *bytes)
{
	ssize_t got = read(handle, buffer, count);
	if (got < 0) {
		if (bytes) {
			*bytes = 0;
		}
		return errno;
	}
	if (bytes) {
		*bytes = static_cast<unsigned>(got);
	}
	return 0;
}

extern "C" unsigned _dos_write(int handle, void const *buffer, unsigned count, unsigned *bytes)
{
	ssize_t wrote = write(handle, buffer, count);
	if (wrote < 0) {
		if (bytes) {
			*bytes = 0;
		}
		return errno;
	}
	if (bytes) {
		*bytes = static_cast<unsigned>(wrote);
	}
	return 0;
}

extern "C" int getdisk(void)
{
	return 2;
}

extern "C" void setdisk(int)
{
}

extern "C" unsigned _harderr(void (*)())
{
	return 0;
}

extern "C" int int86(int, union REGS *inregs, union REGS *outregs)
{
	if (outregs && inregs) {
		*outregs = *inregs;
	}
	return 0;
}

extern "C" int _int86(int interrupt_number, union REGS *inregs, union REGS *outregs)
{
	return int86(interrupt_number, inregs, outregs);
}

extern "C" void geninterrupt(int)
{
}

extern "C" void delay(unsigned milliseconds)
{
	usleep(milliseconds * 1000);
}

extern "C" long filelength(int handle)
{
	off_t current = lseek(handle, 0, SEEK_CUR);
	off_t end = lseek(handle, 0, SEEK_END);
	if (current != static_cast<off_t>(-1)) {
		lseek(handle, current, SEEK_SET);
	}
	return static_cast<long>(end);
}

extern "C" long tell(int handle)
{
	return static_cast<long>(lseek(handle, 0, SEEK_CUR));
}

extern "C" int eof(int handle)
{
	off_t current = lseek(handle, 0, SEEK_CUR);
	off_t end = lseek(handle, 0, SEEK_END);
	if (current != static_cast<off_t>(-1)) {
		lseek(handle, current, SEEK_SET);
	}
	return current >= end;
}

extern "C" int sopen(char const *path, int oflag, int shflag, ...)
{
	(void)shflag;
	mode_t mode = S_IREAD | S_IWRITE;
	if (oflag & O_CREAT) {
		va_list args;
		va_start(args, shflag);
		mode = static_cast<mode_t>(va_arg(args, int));
		va_end(args);
	}
	return open(path, oflag, mode);
}

extern "C" int getch(void)
{
	unsigned char c = 0;
	if (read(STDIN_FILENO, &c, 1) == 1) {
		return c;
	}
	return 0;
}

extern "C" int kbhit(void)
{
	return 0;
}
