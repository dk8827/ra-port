#include <dos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fail(char const *message)
{
	fprintf(stderr, "FAIL: %s\n", message);
	return 1;
}

static int write_file(char const *path, char const *text)
{
	FILE *file = fopen(path, "wb");
	if (!file) return 0;
	fwrite(text, 1, strlen(text), file);
	fclose(file);
	return 1;
}

int main(void)
{
	char dirname[] = "/tmp/ra_dos_find_test_XXXXXX";
	if (!mkdtemp(dirname)) return fail("mkdtemp failed");

	char first[512];
	char second[512];
	char pattern[512];
	snprintf(first, sizeof(first), "%s/SC_A.MIX", dirname);
	snprintf(second, sizeof(second), "%s/SC_B.MIX", dirname);
	snprintf(pattern, sizeof(pattern), "%s/SC*.MIX", dirname);

	if (!write_file(first, "one")) return fail("write first failed");
	if (!write_file(second, "second-file")) return fail("write second failed");

	find_t find;
	if (_dos_findfirst(pattern, _A_NORMAL, &find) != 0) return fail("findfirst failed");

	int saw_first = strcmp(find.name, "SC_A.MIX") == 0 && find.size == 3;
	int saw_second = strcmp(find.name, "SC_B.MIX") == 0 && find.size == 11;
	while (_dos_findnext(&find) == 0) {
		if (strcmp(find.name, "SC_A.MIX") == 0 && find.size == 3) saw_first = 1;
		if (strcmp(find.name, "SC_B.MIX") == 0 && find.size == 11) saw_second = 1;
	}
	_dos_findclose(&find);

	unlink(first);
	unlink(second);
	rmdir(dirname);

	if (!saw_first) return fail("find state lost first file size");
	if (!saw_second) return fail("find state lost second file size");
	return 0;
}
