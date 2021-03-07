#pragma once

#include "Header.h"

//
EFI_STATUS EFIAPI DrawRectangle(IN UINTN x0, IN UINTN y0, IN UINTN x1, IN UINTN y1, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *p);

// Struct used to represent a point in 2D space
typedef struct
{
    INTN X;
    INTN Y;
} POINT;

// Defined fill types.
typedef enum _UI_FILL_TYPE
{
    FILL_SOLID,
    FILL_FORWARD_STRIPE,
    FILL_BACKWARD_STRIPE,
    FILL_VERTICAL_STRIPE,
    FILL_HORIZONTAL_STRIPE,
    FILL_CHECKERBOARD,
    FILL_POLKA_SQUARES
} UI_FILL_TYPE;

typedef enum UI_FILL_TYPE _UI_FILL_TYPE;

typedef union
{
    struct
    {
        UINT32 FillColor;
    } SolidFill;

    struct
    {
        UINT32 Color1;
        UINT32 Color2;
        INT32 StripeSize; //Width or Height depending on Stripe type
    } StripeFill;

    struct
    {
        UINT32 Color1;
        UINT32 Color2;
        INT32 CheckboardWidth;
    } CheckerboardFill;

    struct
    {
        UINT32 Color1;
        UINT32 Color2;
        INT32 DistanceBetweenSquares;
        INT32 SquareWidth;
    } PolkaSquareFill;
} UI_FILL_TYPE_STYLE_UNION;

typedef struct
{
    UINT32 BorderColor;
    INT32 BorderWidth;
} UI_BORDER_STYLE;

typedef enum
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
} UI_PLACEMENT;

typedef struct
{
    INT32 Width;
    INT32 Height;
    UI_PLACEMENT Placement;
    UINT32 *PixelData;
} UI_ICON_INFO;

typedef struct
{
    UI_BORDER_STYLE Border;
    UI_FILL_TYPE FillType;
    UI_FILL_TYPE_STYLE_UNION FillTypeInfo;
    UI_ICON_INFO IconInfo;
} UI_STYLE_INFO;

// Rectangle context
typedef struct
{
    POINT UpperLeft;
    UINT32 Width;
    UINT32 Height;
    UINT8 *FrameBufferBase;
    UINTN PixelsPerScanLine; //in framebuffer
    UI_STYLE_INFO StyleInfo;
} UI_RECTANGLE;

typedef struct
{
    UI_RECTANGLE Public;
    INTN FillDataSize;
    UINT8 FillData[0];
} PRIVATE_UI_RECTANGLE;

UI_RECTANGLE *
    EFIAPI
    new_UI_RECTANGLE(
        IN POINT *UpperLeft,
        IN UINT8 *FrameBufferBase,
        IN UINTN PixelsPerScanLine,
        IN UINT32 Width,
        IN UINT32 Height,
        IN UI_STYLE_INFO *StyleInfo);

/*
Method to free all allocated memory of the UI_RECTANGLE
@param this     - ProgressCircle object to draw
*/
VOID
    EFIAPI
    delete_UI_RECTANGLE(
        IN UI_RECTANGLE *this);

/*
Method to draw the rectangle to the framebuffer.
@param this     - UI_RECTANGLE object to draw
*/
VOID
    EFIAPI
    DrawRect(
        IN UI_RECTANGLE *this);

/***  PRIVATE METHODS ***/

/** 
Method checks to see if the StyleInfo is supported by
this implementation of UiRectangle
@retval TRUE:   Supported
@retval FALSE:  Not Supported
**/
BOOLEAN
IsStyleSupported(IN UI_STYLE_INFO *StyleInfo);

/** 
Method returns the private datasize in bytes needed to support this style info for this rectangle
**/
INTN GetFillDataSize(
    IN UINTN Width,
    IN UINTN Height,
    IN UI_STYLE_INFO *StyleInfo);

VOID PRIVATE_Init(
    IN PRIVATE_UI_RECTANGLE *priv);

VOID DrawBorder(
    IN PRIVATE_UI_RECTANGLE *priv);

VOID DrawIcon(
    IN PRIVATE_UI_RECTANGLE *priv);
