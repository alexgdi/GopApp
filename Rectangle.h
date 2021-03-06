#pragma once

#include "Header.h"

//
EFI_STATUS EFIAPI DrawRectangle(IN UINTN x0, IN UINTN y0, IN UINTN x1, IN UINTN y1, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *p);

// Struct used to represent a point in 2D space
struct POINT
{
    INTN X;
    INTN Y;
};

// Defined fill types.
enum UI_FILL_TYPE
{
    FILL_SOLID,
    FILL_FORWARD_STRIPE,
    FILL_BACKWARD_STRIPE,
    FILL_VERTICAL_STRIPE,
    FILL_HORIZONTAL_STRIPE,
    FILL_CHECKERBOARD,
    FILL_POLKA_SQUARES
};

typedef enum UI_FILL_TYPE _UI_FILL_TYPE;

union UI_FILL_TYPE_STYLE_UNION
{
    struct SolidFill
    {
        UINT32 FillColor;
    };

    struct StripeFill
    {
        UINT32 Color1;
        UINT32 Color2;
        INT32 StripeSize; //Width or Height depending on Stripe type
    };

    struct CheckerboardFill
    {
        UINT32 Color1;
        UINT32 Color2;
        INT32 CheckboardWidth;
    };

    struct PolkaSquareFill
    {
        UINT32 Color1;
        UINT32 Color2;
        INT32 DistanceBetweenSquares;
        INT32 SquareWidth;
    };
};

struct UI_BORDER_STYLE
{
    UINT32 BorderColor;
    INT32 BorderWidth;
};

enum UI_PLACEMENT
{
    INVALID_PLACEMENT,
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    MIDDLE_LEFT,
    MIDDLE_CENTER,
    MIDDLE_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

struct UI_ICON_INFO
{
    INT32 Width;
    INT32 Height;
    enum UI_PLACEMENT Placement;
    UINT32 *PixelData;
};

struct UI_STYLE_INFO
{
    struct UI_BORDER_STYLE Border;
    enum UI_FILL_TYPE FillType;
    union UI_FILL_TYPE_STYLE_UNION FillTypeInfo;
    struct UI_ICON_INFO IconInfo;
};

// Rectangle context
struct UI_RECTANGLE
{
    struct POINT UpperLeft;
    UINT32 Width;
    UINT32 Height;
    UINT8 *FrameBufferBase;
    UINTN PixelsPerScanLine; //in framebuffer
    struct UI_STYLE_INFO StyleInfo;
};

struct PRIVATE_UI_RECTANGLE
{
    struct UI_RECTANGLE Public;
    INTN FillDataSize;
    UINT8 FillData[0];
};

struct UI_RECTANGLE *EFIAPI new_UI_RECTANGLE(IN struct POINT *UpperLeft, IN UINT8 *FrameBufferBase, IN UINTN PixelsPerScanLine, IN UINT32 Width, IN UINT32 Height, IN UI_STYLE_INFO *StyleInfo);

/* Method to free all allocated memory of the UI_RECTANGLE */
VOID EFIAPI delete_UI_RECTANGLE(IN struct UI_RECTANGLE *this);

/* Method to draw the rectangle to the framebuffer. */
VOID EFIAPI DrawRect(IN struct UI_RECTANGLE *this);

/***  PRIVATE METHODS ***/
BOOLEAN IsStyleSupported(IN struct UI_STYLE_INFO *StyleInfo);

/**  Method returns the private datasize in bytes needed to support this style info for this rectangle **/
INTN GetFillDataSize(IN UINTN Width, IN UINTN Height, IN struct UI_STYLE_INFO *StyleInfo);

VOID PRIVATE_Init(IN struct PRIVATE_UI_RECTANGLE *priv);

VOID DrawBorder(IN struct PRIVATE_UI_RECTANGLE *priv);

VOID DrawIcon(IN struct PRIVATE_UI_RECTANGLE *priv);
