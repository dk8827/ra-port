#pragma once

#if defined(__ANDROID__)
#include_next <malloc.h>
#else
#include <stdlib.h>
#endif
