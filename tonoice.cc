// Copyright (C) 2021 Erik Corry. All rights reserved.
// Use of this source code is governed by an MIT-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>

#include "noice.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class ColorPopularity {
 public:
  ColorPopularity() : popularity(-1) {}
  void initialize(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    red = r;
    green = g;
    blue = b;
    alpha = a;
    popularity = 0;
  }
  int popularity;
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
};

static const int MAX_COLORS = 10000;
static const int MAX_PALETTE = 31;

static int compare_popularities(const void* a, const void* b) {
  return reinterpret_cast<const ColorPopularity*>(b)->popularity -
         reinterpret_cast<const ColorPopularity*>(a)->popularity;
}

static bool near_enough(unsigned char r1, unsigned char g1, unsigned char b1, unsigned char a1,
                        unsigned char r2, unsigned char g2, unsigned char b2, unsigned char a2) {
  unsigned char rd = r1 - r2 + 2;
  unsigned char gd = g1 - g2 + 2;
  unsigned char bd = b1 - b2 + 2;
  unsigned char ad = a1 - a2 + 2;
  return rd <= 4 && gd <= 4 && bd <= 4 && ad <= 4;
}

// Is p on a line between c1 and c2 in RGB space?
static bool between(ColorPopularity* p, ColorPopularity* c1, ColorPopularity* c2) {
  // Any component outside the range of c1 to c2?
  if (p->red > c1->red && p->red > c2->red) return false;
  if (p->red < c1->red && p->red < c2->red) return false;
  if (p->green > c1->green && p->green > c2->green) return false;
  if (p->green < c1->green && p->green < c2->green) return false;
  if (p->blue > c1->blue && p->blue > c2->blue) return false;
  if (p->blue < c1->blue && p->blue < c2->blue) return false;
  // Find component where c1 and c2 differ most.
  int rd = static_cast<int>(c1->red) - static_cast<int>(c2->red);
  int gd = static_cast<int>(c1->green) - static_cast<int>(c2->green);
  int bd = static_cast<int>(c1->blue) - static_cast<int>(c2->blue);
  if (rd < 0) rd = -rd;
  if (gd < 0) gd = -gd;
  if (bd < 0) bd = -bd;
  double position;  // From 0 to 1.0 where 0 is c1 and 1.0 is c2.
  if (rd >= gd && rd >= bd) {
    // Red is biggest difference.
    position = (p->red - c1->red) * 1.0 / (c2->red - c1->red);
  } else if (gd >= rd && gd >= bd) {
    // Green is biggest difference.
    position = (p->green - c1->green) * 1.0 / (c2->green - c1->green);
  } else {
    // Blue is biggest difference.
    position = (p->blue - c1->blue) * 1.0 / (c2->blue - c1->blue);
  }
  unsigned char exact_red = c1->red + (c2->red - c1->red) * position;
  unsigned char exact_green = c1->green + (c2->green - c1->green) * position;
  unsigned char exact_blue = c1->blue + (c2->blue - c1->blue) * position;
  bool result = near_enough(p->red, p->green, p->blue, p->alpha, exact_red, exact_green, exact_blue, p->alpha);
  if (result) {
    printf("%02x%02x%02x is %d%% between %02x%02x%02x and %02x%02x%02x\n",
        p->red, p->green, p->blue,
        static_cast<int>(position * 100),
        c1->red, c1->green, c1->blue,
        c2->red, c2->green, c2->blue);
  }
  return result;
}

// Is p on a plane between c1, c2, and c3 in RGB space?
static bool between(ColorPopularity* p, ColorPopularity* c1, ColorPopularity* c2, ColorPopularity* c3) {
  // Any component outside the range of c1 to c2 or c3?
  if (p->red > c1->red && p->red > c2->red && p->red > c3->red) return false;
  if (p->red < c1->red && p->red < c2->red && p->red < c3->red) return false;
  if (p->green > c1->green && p->green > c2->green && p->green > c3->green) return false;
  if (p->green < c1->green && p->green < c2->green && p->green < c3->green) return false;
  if (p->blue > c1->blue && p->blue > c2->blue && p->blue > c3->blue) return false;
  if (p->blue < c1->blue && p->blue < c2->blue && p->blue < c3->blue) return false;
  return false;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: tonoice input output.noice\n");
    return 1;
  }
  int width, height, channels_in_file;
  unsigned char *image = stbi_load(argv[1], &width, &height, &channels_in_file, 4);
  if (image == 0) {
    perror(argv[1]);
    exit(2);
  }
  ColorPopularity popularities[MAX_COLORS];
  int unused_index = 0;
  for (int i = 0; i < width * height; i++) {
    unsigned char r = image[i * 4];
    unsigned char g = image[i * 4 + 1];
    unsigned char b = image[i * 4 + 2];
    unsigned char a = image[i * 4 + 3];
    if (a < 10) {
      a = r = g = b = 0;  // Normalize all transparent colors.
    } else if (a > 245) {
      a = 255;
    }
    for (int j = 0; j < MAX_COLORS; j++) {
      ColorPopularity* p = popularities + j;
      if (near_enough(r, g, b, a, p->red, p->green, p->blue, p->alpha)) {
        p->popularity++;
        break;
      }
      if (p->popularity == -1) {
        p->initialize(r, g, b, a);
        p->popularity++;
        unused_index = j + 1;
        break;
      }
      if (j == MAX_COLORS - 1) {
        fprintf(stderr, "Warning - ignored #%02x%02x%02x alpha %02x\n", r, g, b, a);
      }
    }
  }
  qsort(popularities, unused_index, sizeof(ColorPopularity), compare_popularities);
  ColorPopularity palette[MAX_PALETTE];
  int palette_count = 0;
  for (int i = 0; i < unused_index; i++) {
    ColorPopularity* p = popularities + i;
    if (p->alpha == 0) continue;
    if (p->alpha == 255) {
      bool found = false;
      for (int j = 0; j < palette_count && !found; j++) {
        ColorPopularity* c1 = palette + j;
        for (int k = j + 1; k < palette_count; k++) {
          ColorPopularity* c2 = palette + k;
          if (between(p, c1, c2)) {
            found = true;
            break;
          }
        }
      }
      if (!found) {
        palette[palette_count].initialize(p->red, p->green, p->blue, p->alpha);
        palette[palette_count++].popularity = p->popularity;
      }
    }
  }

  for (int i = 0; i < palette_count; i++) {
    ColorPopularity* p = palette + i;
    printf("%02x%02x%02x %02x %d\n", p->red, p->green, p->blue, p->alpha, p->popularity);
  }
}
