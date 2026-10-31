// Exercise the real filesystem bootstrap without launching the game or UIKit.
#include "../PORT/IOS/src/ios_main.cpp"
#include <assert.h>

int PASCAL WinMain(HINSTANCE, HINSTANCE, char *, int) { return 0; }

static void write_asset(std::string const &path, int size, int value)
{
	FILE *file = fopen(path.c_str(), "wb");
	assert(file);
	for (int i = 0; i < size; ++i) assert(fputc(value, file) != EOF);
	assert(fclose(file) == 0);
}

static int first_byte(std::string const &path)
{
	FILE *file = fopen(path.c_str(), "rb");
	assert(file);
	int value = fgetc(file);
	fclose(file);
	return value;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	std::string root = argv[1];
	std::string old_bundle = root + "/old", new_bundle = root + "/new", data = root + "/data";
	assert(make_directories(old_bundle));
	assert(make_directories(new_bundle));
	assert(make_directories(data));
	write_asset(old_bundle + "/large.mix", 65536, 1);
	write_asset(old_bundle + "/shrinking.cps", 65536, 2);
	write_asset(old_bundle + "/options.ini", 4, 3);
	write_asset(data + "/large.mix", 65536, 1); // migrate an earlier full copy
	assert(sync_directory_recursive(old_bundle.c_str(), data.c_str()));
	struct stat info;
	assert(lstat((data + "/large.mix").c_str(), &info) == 0 && S_ISLNK(info.st_mode));
	write_asset(data + "/options.ini", 4, 9); // user settings must survive
	write_asset(new_bundle + "/large.mix", 65537, 4);
	write_asset(new_bundle + "/shrinking.cps", 65535, 5);
	write_asset(new_bundle + "/options.ini", 4, 6);
	assert(unlink((old_bundle + "/large.mix").c_str()) == 0);
	assert(unlink((old_bundle + "/shrinking.cps").c_str()) == 0);
	assert(sync_directory_recursive(new_bundle.c_str(), data.c_str()));
	assert(first_byte(data + "/large.mix") == 4);
	assert(first_byte(data + "/shrinking.cps") == 5);
	assert(lstat((data + "/shrinking.cps").c_str(), &info) == 0 && S_ISREG(info.st_mode));
	assert(first_byte(data + "/options.ini") == 9);
	assert(sync_directory_recursive(new_bundle.c_str(), data.c_str()));
	assert(first_byte(data + "/options.ini") == 9);
	// A failed replacement must leave the existing destination intact.
	assert(!copy_file((root + "/missing").c_str(), (data + "/options.ini").c_str()));
	assert(first_byte(data + "/options.ini") == 9);
	assert(make_directories(data + "/blocked"));
	assert(!link_file((new_bundle + "/large.mix").c_str(), (data + "/blocked").c_str()));
	assert(directory_exists((data + "/blocked").c_str()));
	return 0;
}
