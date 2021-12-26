/**
0  4   magic
4  3              flags (8, 16, or 24 bit pointers)
7  1              palette size (always 0, 4, 8, 12, or 16).  (16 is always transparent, 0 palette is a black/transparent image).
8  4              file size
12 2              width
14 2              height
16 0/12/24/36/48/60/72/84/96  Palette of up to 32, 16th is always transparent.
16/18/30/42/64/80/96/112    end of header
*/

#pragma once

static const unsigned int MAGIC = 0x901ce;

class Header {
 public:
  unsigned int magic;
  unsigned short flags;
  unsigned char bits_per_pointer;  // 8 16 or 24.
  unsigned char palette_size;      // Multiple of 4, up to 32, color 15 is always transparent
  unsigned int file_size;
  unsigned int width;
  unsigned int height;
  unsigned char palette[1];        // Actually 3 bytes per entry.
};


