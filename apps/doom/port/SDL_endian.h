#ifndef DOOM_SDL_ENDIAN_H_
#define DOOM_SDL_ENDIAN_H_

#include <stdint.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "This Chocolate Doom port runs on a little-endian CPU"
#endif

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321
#define SDL_BYTEORDER  SDL_LIL_ENDIAN

#define SDL_SwapLE16(x) ((uint16_t)(x))
#define SDL_SwapLE32(x) ((uint32_t)(x))
#define SDL_SwapBE16(x) \
    ((uint16_t)((((uint16_t)(x) & 0xffu) << 8) | (((uint16_t)(x) >> 8) & 0xffu)))
#define SDL_SwapBE32(x) \
    ((uint32_t)((((uint32_t)(x) & 0xffu) << 24) | (((uint32_t)(x) & 0xff00u) << 8) \
                | (((uint32_t)(x) >> 8) & 0xff00u) | (((uint32_t)(x) >> 24) & 0xffu)))

#endif
