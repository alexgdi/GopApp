#pragma once

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Protocol/HiiFont.h>
#include <Protocol/SimplePointer.h>
//#include <Library/HiiLib.h>

// FILE
#define BREAK_ERR(x)        \
    if (EFI_SUCCESS != (x)) \
    {                       \
        break;              \
    }

//
EFI_GRAPHICS_OUTPUT_PROTOCOL *gGop;

//
EFI_HII_FONT_PROTOCOL *gHiiFontProtocol;

//
EFI_SIMPLE_POINTER_PROTOCOL *gSpp;
