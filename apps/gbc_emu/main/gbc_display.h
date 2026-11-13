/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file gbc_display.h
 * @brief Scale a 160x144 frame onto the 410x502 panel.
 */
#ifndef GBC_DISPLAY_H_
#define GBC_DISPLAY_H_

#include <stdbool.h>
#include <stdint.h>

#include "gbc_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The picture ---- */

/** @brief Game Boy width. */
#define GBC_LCD_W      160U

/** @brief Game Boy height. */
#define GBC_LCD_H      144U

/** @brief Integer scale that fits the glass with room for Start and Select. */
#define GBC_LCD_SCALE  2U

/** @brief Pixels in one 160x144 RGB565 frame. */
#define GBC_LCD_PIXELS (GBC_LCD_W * GBC_LCD_H)

/* ---- Calls ---- */

/**
 * @brief Draw one RGB565 frame at 2x, then the Start and Select zones, and flip.
 *
 * Writes the panel's back buffer and flushes it. The first call waits until
 * AO_Display has brought the panel up. A later call waits out the previous
 * flip before it draws, so it does not paint a buffer the DMA is still reading.
 *
 * This calls the display driver directly. kode-apps rule C3 warns on that
 * include. The kode screen widgets cannot hold 59.7 Hz, and Kody Kart already
 * takes this path for the same reason.
 *
 * @param lcd  160x144 RGB565, row major, little-endian. NULL draws nothing.
 * @param pad  Lights a zone while start or select is set. NULL leaves both dark.
 * @return true when a frame was queued to the panel.
 */
bool gbc_display_present(const uint16_t *lcd, const gbc_pad_t *pad);

#ifdef __cplusplus
}
#endif

#endif /* GBC_DISPLAY_H_ */
