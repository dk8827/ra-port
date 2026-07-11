#include "SAVEDESCRIPTION.H"

#include <assert.h>
#include <string.h>

int main(void)
{
	char normal[16] = "Mission\r\n";
	Normalize_Save_Description(normal, sizeof(normal));
	assert(strcmp(normal, "Mission") == 0);

	char full[44];
	memset(full, 'A', sizeof(full));
	Normalize_Save_Description(full, sizeof(full));
	assert(full[43] == '\0');
	assert(strlen(full) == 43);

	char short_value[2] = "A";
	Normalize_Save_Description(short_value, sizeof(short_value));
	assert(strcmp(short_value, "A") == 0);
	return 0;
}
