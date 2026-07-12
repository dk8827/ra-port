#ifndef MAC_SDL_H
#define MAC_SDL_H

#define SDL_MAIN_HANDLED

/*
 * The legacy game target defines WIN32 even on portable desktop builds.
 * Hide it while SDL chooses its platform headers, then restore it for the
 * surrounding compatibility layer.
 */
#ifdef WIN32
#define MAC_SDL_RESTORE_WIN32 1
#undef WIN32
#endif

#include <SDL.h>

#ifdef MAC_SDL_RESTORE_WIN32
#define WIN32 1
#undef MAC_SDL_RESTORE_WIN32
#endif

#endif
