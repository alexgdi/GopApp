#include "Header.h"

#include "Rectangle.h"

// #include <PiDxe.h>
// #include <Library/BaseMemoryLib.h>
// #include <Library/DebugLib.h>
// #include <Protocol/GraphicsOutput.h> //structure defs
// #include <Library/MemoryAllocationLib.h>

EFI_STATUS EFIAPI DrawRectangle(IN UINTN x0, IN UINTN y0, IN UINTN x1, IN UINTN y1, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *p)
{
    EFI_STATUS Status;

    Status = gGop->Blt(gGop, p, EfiBltVideoFill, 0, 0, x0, y0, x1, y1, 0);

    if (EFI_ERROR(Status))
    {
        Print(L"Unable to DrawRectangle in %d %d %d %d\n", x0, y0, x1, y1);
        return EFI_ERROR(-1);
    }

    return EFI_SUCCESS;
}

/* Method to use create a new UI_RECTANGLE struct. This structure is used by all the other functions to modify and draw the object */
struct UI_RECTANGLE *EFIAPI new_UI_RECTANGLE(IN struct POINT *UpperLeft, IN UINT8 *FrameBufferBase, IN UINTN PixelsPerScanLine, IN UINT32 Width, IN UINT32 Height,
                                      IN struct UI_STYLE_INFO *StyleInfo)
{
    INTN FillDataSize = 0;

    //
    if (FrameBufferBase == NULL)
    {
        ASSERT(NULL != FrameBufferBase);
        return NULL;
    }

    // Проверка точки рисования
    if (UpperLeft == NULL)
    {
        ASSERT(NULL != UpperLeft);
        return NULL;
    }

    // Проверка поддержки стиля
    if (!IsStyleSupported(StyleInfo))
    {
        DEBUG((DEBUG_ERROR, "Style Info requested by caller is not supported!"));
        return NULL;
    }

    // Вычисление размера заполнения с учётом стиля?
    FillDataSize = GetFillDataSize(Width, Height, StyleInfo);

    // Выделяем память
    PRIVATE_UI_RECTANGLE *this = (PRIVATE_UI_RECTANGLE *)AllocateZeroPool(sizeof(PRIVATE_UI_RECTANGLE) + FillDataSize);

    //
    ASSERT(NULL != this);

    // Заполнение структуры
    if (this != NULL)
    {
        this->Public.UpperLeft = *UpperLeft;
        this->Public.FrameBufferBase = FrameBufferBase;
        this->Public.PixelsPerScanLine = PixelsPerScanLine;
        this->Public.Width = Width;
        this->Public.Height = Height;
        this->Public.StyleInfo = *StyleInfo;
        this->FillDataSize = FillDataSize;
        this->Public.StyleInfo.IconInfo.PixelData = NULL;

        // Иконка?
        if ((this->Public.StyleInfo.IconInfo.Height > 0) && (this->Public.StyleInfo.IconInfo.Width > 0) && (StyleInfo->IconInfo.PixelData != NULL))
        {
            INTN PixelDataSize = this->Public.StyleInfo.IconInfo.Height * this->Public.StyleInfo.IconInfo.Width * sizeof(UINT32);
            this->Public.StyleInfo.IconInfo.PixelData = (UINT32 *)AllocatePool(PixelDataSize);

            if (this->Public.StyleInfo.IconInfo.PixelData != NULL)
            {
                CopyMem(this->Public.StyleInfo.IconInfo.PixelData, StyleInfo->IconInfo.PixelData, PixelDataSize);
            }
        }

        // Иннициализация
        PRIVATE_Init(this);

        return &this->Public;
    }

    return NULL;
}

/* Method to free all allocated memory of the UI_RECTANGLE */
VOID EFIAPI delete_UI_RECTANGLE(IN struct UI_RECTANGLE *this)
{
    if (this != NULL)
    {
        if (this->StyleInfo.IconInfo.PixelData != NULL)
        {
            FreePool(this->StyleInfo.IconInfo.PixelData);
        }
        FreePool(this);
    }
}

/* Method to draw the rectangle to the framebuffer. */
VOID EFIAPI DrawRect(IN struct UI_RECTANGLE *this)
{
    PRIVATE_UI_RECTANGLE *priv = (PRIVATE_UI_RECTANGLE *)this;
    UINT8 *StartAddressInFrameBuffer = (UINT8 *)(((UINT32 *)this->FrameBufferBase) + ((this->UpperLeft.Y * this->PixelsPerScanLine) + this->UpperLeft.X));
    UINTN RowLengthInBytes = this->Width * sizeof(UINT32);

    for (INTN y = 0; y < (INTN)this->Height; y++) //each row
    {
        UINT32 *temp = NULL;
        switch (this->StyleInfo.FillType)
        {

        case FILL_SOLID:
        case FILL_VERTICAL_STRIPE:
            temp = (UINT32 *)priv->FillData;
            break;

        case FILL_HORIZONTAL_STRIPE:
            temp = (UINT32 *)priv->FillData; //set to first row...else set to 2nd row
            if (((y / priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize) % 2) == 0)
            {
                //temp is uint32 pointer so move it forward by pixels
                temp += priv->Public.Width;
            }
            break;

        case FILL_FORWARD_STRIPE:
            temp = (UINT32 *)priv->FillData;
            temp += (y % priv->Public.Height); //Add 1 each time
            break;

        case FILL_BACKWARD_STRIPE:
            temp = (UINT32 *)priv->FillData;
            temp += (priv->Public.Height - (y % priv->Public.Height));
            break;

        case FILL_CHECKERBOARD:
            temp = (UINT32 *)priv->FillData;
            if (((y / priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth) % 2) == 0)
            {
                temp += priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth;
            }
            break;

        case FILL_POLKA_SQUARES:
            temp = (UINT32 *)priv->FillData; //set to first row

            if (((y + (priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares / 2)) % (priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares + priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth)) > priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares)
            {
                //temp is uint32 pointer so move it forward by pixels
                temp += priv->Public.Width;
            }
            break;

        default:
            DEBUG((DEBUG_ERROR, "Unsupported Fill Type.  Cant draw Rectangle  0x%X\n", this->StyleInfo.FillType));
            return;
        }

        CopyMem(StartAddressInFrameBuffer, temp, RowLengthInBytes);
        StartAddressInFrameBuffer += (this->PixelsPerScanLine * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)); //move to next row
    }

    // Нарисовать границу если есть
    if (this->StyleInfo.Border.BorderWidth > 0)
    {
        DrawBorder(priv);
    }

    // Нарисовать иконку если есть
    if (this->StyleInfo.IconInfo.PixelData != NULL)
    {
        DrawIcon(priv);
    }
}

/***  PRIVATE METHODS ***/

/** Method checks to see if the StyleInfo is supported by this implementation of UiRectangle **/
BOOLEAN IsStyleSupported(IN struct UI_STYLE_INFO *StyleInfo)
{
    if ((StyleInfo->FillType > FILL_POLKA_SQUARES) || (StyleInfo->FillType < FILL_SOLID))
    {
        DEBUG((DEBUG_ERROR, "Unsupported FillType 0x%X\n", (UINTN)StyleInfo->FillType));
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/** Method returns the private datasize in bytes needed to support this style info for this rectangle **/
INTN GetFillDataSize(IN UINTN Width, IN UINTN Height, IN struct UI_STYLE_INFO *StyleInfo)
{
    INTN PixelBufferLength = 0;

    switch (StyleInfo->FillType)
    {
    case FILL_SOLID:
    case FILL_VERTICAL_STRIPE:
        PixelBufferLength = Width;
        break;

    case FILL_HORIZONTAL_STRIPE:
    case FILL_POLKA_SQUARES:
        PixelBufferLength = Width * 2;
        break;

    case FILL_FORWARD_STRIPE:
    case FILL_BACKWARD_STRIPE:
        PixelBufferLength = Width + Height;
        break;

    case FILL_CHECKERBOARD:
        PixelBufferLength = Width + StyleInfo->FillTypeInfo.CheckerboardFill.CheckboardWidth;
        break;
    }

    return PixelBufferLength * sizeof(UINT32); //each pixel is a uint32
}

//
VOID PRIVATE_Init(IN struct PRIVATE_UI_RECTANGLE *priv)
{
    // Init any private data needed for this rectangle
    INTN FillDataSizeInPixels = priv->FillDataSize / sizeof(UINT32);

    // Private fill data is used to hold row data needed for the fill
    switch (priv->Public.StyleInfo.FillType)
    {
    // сплошная заливка
    case FILL_SOLID:
        SetMem32(priv->FillData, priv->FillDataSize, priv->Public.StyleInfo.FillTypeInfo.SolidFill.FillColor);
        break;

    // горизонтальные полоски
    case FILL_HORIZONTAL_STRIPE:
        SetMem32(priv->FillData, (priv->FillDataSize / 2), priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color1);
        SetMem32(priv->FillData + (priv->Public.Width * sizeof(UINT32)), (priv->FillDataSize / 2), priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color2);
        break;

        // вертикальные полоски
    case FILL_FORWARD_STRIPE:
    case FILL_BACKWARD_STRIPE:
    case FILL_VERTICAL_STRIPE:
        SetMem32(priv->FillData, priv->FillDataSize, priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color1); //set row to Color1

        //setup alternate color.  Color band is width stripe_width.
        //i is in pixels not bytes
        for (INTN i = priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize; i < FillDataSizeInPixels; i += (priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize * 2))
        {
            INTN Len = FillDataSizeInPixels - i;
            if (Len > priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize)
            {
                Len = (INTN)(priv->Public.StyleInfo.FillTypeInfo.StripeFill.StripeSize);
            }
            Len = Len * sizeof(UINT32); //Convert Len from Pixels to bytes
            SetMem32(((UINT32 *)priv->FillData) + i, Len, priv->Public.StyleInfo.FillTypeInfo.StripeFill.Color2);
        }
        break;
        // в шахматном порядке
    case FILL_CHECKERBOARD:
        SetMem32(priv->FillData, priv->FillDataSize, priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.Color1); //set row to Color1

        //setup alternate color.  Color band is width stripe_width.
        for (int i = priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth; i < FillDataSizeInPixels; i += (priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth * 2))
        {
            INTN Len = FillDataSizeInPixels - i;
            if (Len > priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth)
            {
                Len = (INTN)priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.CheckboardWidth;
            }
            Len = Len * sizeof(UINT32); //convert len from Pixels to bytes
            SetMem32(((UINT32 *)priv->FillData) + i, Len, priv->Public.StyleInfo.FillTypeInfo.CheckerboardFill.Color2);
        }
        break;

        //
    case FILL_POLKA_SQUARES:
        SetMem32(priv->FillData, priv->FillDataSize, priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.Color1); //set row to Color1

        //setup dot row. as row two of the filldata
        for (int i = priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares / 2;
             i < (FillDataSizeInPixels / 2);
             i += (priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth + priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.DistanceBetweenSquares))
        {
            INTN Len = (FillDataSizeInPixels / 2) - i;
            if (Len > priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth)
            {
                Len = (INTN)priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.SquareWidth;
            }
            Len = Len * sizeof(UINT32);                                                                                                                  //convert len from Pixels to bytes
            SetMem32(priv->FillData + (priv->FillDataSize / 2) + (i * sizeof(UINT32)), Len, priv->Public.StyleInfo.FillTypeInfo.PolkaSquareFill.Color2); //color only the second row
        }
        break;

    default:
        DEBUG((DEBUG_ERROR, "Unsupported Fill Type.  0x%X\n", priv->Public.StyleInfo.FillType));
        break;
    }
}

/** Method used to draw the rectangle border.  Border Width is included in rectangle width. **/
VOID DrawBorder(IN struct PRIVATE_UI_RECTANGLE *priv)
{
    UINT32 *TempFB = (UINT32 *)priv->Public.FrameBufferBase;                                            //frame buffer pointer
    TempFB += ((priv->Public.UpperLeft.Y * priv->Public.PixelsPerScanLine) + priv->Public.UpperLeft.X); //move ptr to upper left of uirect

    if (priv->Public.StyleInfo.Border.BorderWidth <= 0)
    {
        return;
    }

    //Draw borders
    for (INTN Y = 0; Y < (INTN)priv->Public.Height; Y++)
    {
        if ((Y >= priv->Public.StyleInfo.Border.BorderWidth) && (Y < (INTN)(priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth)))
        {
            //left border
            SetMem32(TempFB, priv->Public.StyleInfo.Border.BorderWidth * sizeof(UINT32), priv->Public.StyleInfo.Border.BorderColor);
            //right border
            SetMem32(TempFB + (priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth), priv->Public.StyleInfo.Border.BorderWidth * sizeof(UINT32), priv->Public.StyleInfo.Border.BorderColor);
        }
        else
        {
            //top or bottom border
            SetMem32(TempFB, priv->Public.Width * sizeof(UINT32), priv->Public.StyleInfo.Border.BorderColor);
        }
        TempFB += priv->Public.PixelsPerScanLine;
    }
    return;
}

/**
Method used to draw an icon in the rectangle.
**/
VOID DrawIcon(IN struct PRIVATE_UI_RECTANGLE *priv)
{
    INT32 OffsetX = 0; //Left edge of icon in coordinate space of the rectangle
    INT32 OffsetY = 0; //Upper edge of icon in coordinate space of the rectangle
    INT32 SizeX = (priv->Public.Width - (priv->Public.StyleInfo.Border.BorderWidth * 2));
    INT32 SizeY = (priv->Public.Height - (priv->Public.StyleInfo.Border.BorderWidth * 2));
    UINT32 *TempFB = (UINT32 *)priv->Public.FrameBufferBase; //frame buffer pointer
    UINT32 *TempIcon = priv->Public.StyleInfo.IconInfo.PixelData;

    if (priv->Public.StyleInfo.IconInfo.PixelData != NULL)
    {
        if ((priv->Public.StyleInfo.IconInfo.Width > SizeX) || (priv->Public.StyleInfo.IconInfo.Height > SizeY))
        {
            DEBUG((DEBUG_ERROR, "Icon is larger than UI Rectangle.  Can't display icon.\n"));
            return;
        }

        //
        // Figure out where the logo is placed based on rect size, border, icon size, and icon placement
        // Offset X and Y will be in coordinate space of rectangle.
        //
        switch (priv->Public.StyleInfo.IconInfo.Placement)
        {
        case TOP_LEFT:
            OffsetX = priv->Public.StyleInfo.Border.BorderWidth;
            OffsetY = priv->Public.StyleInfo.Border.BorderWidth;
            break;

        case TOP_CENTER:
            OffsetX = (priv->Public.Width / 2) - (priv->Public.StyleInfo.IconInfo.Width / 2);
            OffsetY = priv->Public.StyleInfo.Border.BorderWidth;
            break;

        case TOP_RIGHT:
            OffsetX = priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Width;
            OffsetY = priv->Public.StyleInfo.Border.BorderWidth;
            break;

        case MIDDLE_LEFT:
            OffsetX = priv->Public.StyleInfo.Border.BorderWidth;
            OffsetY = (priv->Public.Height / 2) - (priv->Public.StyleInfo.IconInfo.Height / 2);
            break;

        case MIDDLE_CENTER:
            OffsetX = (priv->Public.Width / 2) - (priv->Public.StyleInfo.IconInfo.Width / 2);
            OffsetY = (priv->Public.Height / 2) - (priv->Public.StyleInfo.IconInfo.Height / 2);
            break;

        case MIDDLE_RIGHT:
            OffsetX = priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Width;
            OffsetY = (priv->Public.Height / 2) - (priv->Public.StyleInfo.IconInfo.Height / 2);
            break;

        case BOTTOM_LEFT:
            OffsetX = priv->Public.StyleInfo.Border.BorderWidth;
            OffsetY = priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Height;
            break;

        case BOTTOM_CENTER:
            OffsetX = (priv->Public.Width / 2) - (priv->Public.StyleInfo.IconInfo.Width / 2);
            OffsetY = priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Height;
            break;

        case BOTTOM_RIGHT:
            OffsetX = priv->Public.Width - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Width;
            OffsetY = priv->Public.Height - priv->Public.StyleInfo.Border.BorderWidth - priv->Public.StyleInfo.IconInfo.Height;
            break;

        default:
            DEBUG((DEBUG_ERROR, "Unsupported Icon Placement value.\n"));
            return;
        } //close switch

        //now convert offsetx and y to framebuffer coordinates and draw it
        TempFB += ((priv->Public.UpperLeft.Y * priv->Public.PixelsPerScanLine) + priv->Public.UpperLeft.X); //move ptr to upper left of uirect
        TempFB += ((OffsetY * priv->Public.PixelsPerScanLine) + OffsetX);                                   //move ptr to upper left of icon
        for (INTN Y = 0; Y < priv->Public.StyleInfo.IconInfo.Height; Y++)
        {
            //draw icon
            CopyMem(TempFB, TempIcon, priv->Public.StyleInfo.IconInfo.Width * sizeof(UINT32));
            TempFB += priv->Public.PixelsPerScanLine;
            TempIcon += priv->Public.StyleInfo.IconInfo.Width;
        }

    } //close if pixel data not null
}