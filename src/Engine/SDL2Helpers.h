#pragma once
#include <SDL_version.h>
#include <SDL_rwops.h>

/*
 * Bits of SDL2 API that are missing from SDL1
 * Implementations are in FileMap.cpp
 * To be removed with transition to SDL2
 */
#if SDL_MAJOR_VERSION == 1
extern "C"
{
// helpers that are already present in SDL2
Uint8 SDL_ReadU8(SDL_RWops *src);
Sint64 SDL_RWsize(SDL_RWops *src);
void *SDL_LoadFile_RW(SDL_RWops *src, size_t *datasize, int freesrc);
}
#endif
