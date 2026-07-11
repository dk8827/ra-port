#include <stddef.h>
#include <stdio.h>

static int constructor_calls = 0;

class NullPoolObject {
public:
	NullPoolObject() { constructor_calls++; }
	static void *operator new(size_t) throw() { return NULL; }
};

int main(void)
{
	NullPoolObject *object = new NullPoolObject;
	if (object != NULL || constructor_calls != 0) {
		fprintf(stderr, "FAIL: null pool allocation invoked construction\n");
		return 1;
	}
	return 0;
}
