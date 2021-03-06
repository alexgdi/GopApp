#pragma once

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

// Predefined colors
#define MS_GRAPHICS_WHITE_COLOR                                    \
    {                                                              \
        .Blue = 0xFF, .Green = 0xFF, .Red = 0xFF, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_BLACK_COLOR                                    \
    {                                                              \
        .Blue = 0x00, .Green = 0x00, .Red = 0x00, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_GREEN_COLOR                                    \
    {                                                              \
        .Blue = 0x00, .Green = 0xFF, .Red = 0x00, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_YELLOW_COLOR                                   \
    {                                                              \
        .Blue = 0x00, .Green = 0xFF, .Red = 0xFF, .Reserved = 0xFF \
    }

#define MS_GRAPHICS_LIGHT_GRAY_1_COLOR                             \
    {                                                              \
        .Blue = 0xF2, .Green = 0xF2, .Red = 0xF2, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_LIGHT_GRAY_2_COLOR                             \
    {                                                              \
        .Blue = 0xE6, .Green = 0xE6, .Red = 0xE6, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_LIGHT_GRAY_3_COLOR                             \
    {                                                              \
        .Blue = 0xCC, .Green = 0xCC, .Red = 0xCC, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_MED_GRAY_1_COLOR                               \
    {                                                              \
        .Blue = 0x3A, .Green = 0x3A, .Red = 0x3A, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_MED_GRAY_2_COLOR                               \
    {                                                              \
        .Blue = 0x99, .Green = 0x99, .Red = 0x99, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_MED_GRAY_3_COLOR                               \
    {                                                              \
        .Blue = 0x7A, .Green = 0x7A, .Red = 0x7A, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_DARK_GRAY_1_COLOR                              \
    {                                                              \
        .Blue = 0x73, .Green = 0x73, .Red = 0x73, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_DARK_GRAY_2_COLOR                              \
    {                                                              \
        .Blue = 0x33, .Green = 0x33, .Red = 0x33, .Reserved = 0xFF \
    }

#define MS_GRAPHICS_CYAN_COLOR                                     \
    {                                                              \
        .Blue = 0xD7, .Green = 0x78, .Red = 0x00, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_RED_COLOR                                      \
    {                                                              \
        .Blue = 0x0D, .Green = 0x00, .Red = 0xAE, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_TEXT_RED_COLOR                                 \
    {                                                              \
        .Blue = 0x00, .Green = 0x33, .Red = 0xFF, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_LIGHT_RED_COLOR                                \
    {                                                              \
        .Blue = 0x21, .Green = 0x11, .Red = 0xE8, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_MED_GREEN_COLOR                                \
    {                                                              \
        .Blue = 0x50, .Green = 0x9D, .Red = 0x45, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_LIGHT_CYAN_COLOR                               \
    {                                                              \
        .Blue = 0xF7, .Green = 0xE4, .Red = 0xCC, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_LIGHT_BLUE_COLOR                               \
    {                                                              \
        .Blue = 0xE8, .Green = 0xA2, .Red = 0x00, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_BRIGHT_BLUE_COLOR                              \
    {                                                              \
        .Blue = 0xEA, .Green = 0xD9, .Red = 0x99, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_MED_BLUE_COLOR                                 \
    {                                                              \
        .Blue = 0xE7, .Green = 0xC1, .Red = 0x91, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_SKY_BLUE_COLOR                                 \
    {                                                              \
        .Blue = 0xB2, .Green = 0x67, .Red = 0x20, .Reserved = 0xFF \
    }

#define MS_GRAPHICS_NEAR_BLACK_COLOR                               \
    {                                                              \
        .Blue = 0x1A, .Green = 0x1A, .Red = 0x1A, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_GRAY_KEY_FILL_COLOR                            \
    {                                                              \
        .Blue = 0x4D, .Green = 0x4D, .Red = 0x4D, .Reserved = 0xFF \
    }
#define MS_GRAPHICS_DARK_GRAY_KEY_FILL_COLOR                       \
    {                                                              \
        .Blue = 0x33, .Green = 0x33, .Red = 0x33, .Reserved = 0xFF \
    }

#define COLOR_RED (0xFFfb0200)
#define COLOR_ORANGE (0xFFfd6802)
#define COLOR_YELLOW (0xFFffef00)
#define COLOR_GREEN (0xFF00ff03)
#define COLOR_BLUE (0xFF0094fb)
#define COLOR_INDIGO (0xFF4500f7)
#define COLOR_VIOLET (0xFF9c00ff)
#define COLOR_BROWN (0xFF654321)

#define COLOR_GREY (0xFFC0C0C0)
#define COLOR_DARK_GREY (0xFF404040)
#define COLOR_BLACK (0xFF000000)
#define COLOR_WHITE (0xFFFFFFFF)