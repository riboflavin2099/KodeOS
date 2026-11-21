#ifndef __I_SWAP__
#define __I_SWAP__

#include "SDL_endian.h"

#define SHORT(x) ((signed short)SDL_SwapLE16(x))
#define LONG(x)  ((signed int)SDL_SwapLE32(x))

#endif
