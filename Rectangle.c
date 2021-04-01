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