#ifndef CONSTANTS_H
#define CONSTANTS_H

#if defined(PLATFORM_DESKTOP)
	#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
	#define GLSL_VERSION 100
#endif

namespace Constants {
	static int glsl_version = GLSL_VERSION;
}

#endif