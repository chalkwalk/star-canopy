// The one translation unit that compiles tinyexr's implementation, with the
// miniz beside it for ZIP compression.
#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 1
#define TINYEXR_USE_THREAD 0
#include "tinyexr.h"
