// The single raygui implementation for the whole program. Other files include <raygui.h> without the define.
#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-function"	// GuiFontIconBaking (raygui, see Phase 0 devlog)
#endif

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#if defined(__clang__)
	#pragma clang diagnostic pop
#endif
