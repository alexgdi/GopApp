#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

// The Entry Point for Application. The user code starts with this function
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status;
	EFI_HANDLE *handles[10];
	UINTN handleCount;

	// Очистить экран
	gST->ConOut->ClearScreen(gST->ConOut);

	// Получить дескрипторы которые поддерживают BlockIo протокол
	Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &handleCount, handles);

	if (EFI_ERROR(Status))
	{
		Print(L"LocateHandleBuffer error!\n");
		return EFI_ERROR(-1);
	}

	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop[10];
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;

	Print(L"handleCount = %d\n", handleCount);

	// костыль
	for (UINTN i = 0; i < handleCount; i++)
	{
		Status = gBS->HandleProtocol(*handles[i], &gEfiGraphicsOutputProtocolGuid, (void **)&gop[i]);

		if (EFI_ERROR(Status))
		{
			Print(L"HandleProtocol error!\n");
			break;
		}

		UINTN SizeOfInfo, numModes, nativeMode;

		Status = gop[i]->QueryMode(gop[i], gop[i]->Mode == NULL ? 0 : gop[i]->Mode->Mode, &SizeOfInfo, &info);

		// this is needed to get the current video mode
		//if (Status == EFI_NOT_STARTED)
		//Status = Gop->SetMode(Gop, 0);

		if (EFI_ERROR(Status))
		{
			Print(L"Unable to get native mode\n");
		}
		else
		{
			nativeMode = gop[i]->Mode->Mode;
			numModes = gop[i]->Mode->MaxMode;
			Print(L"numModes = %d nativeMode = %d\n", numModes, nativeMode);
		}

		for (UINTN j = 0; j < numModes; j++)
		{
			Status = gop[i]->QueryMode(gop[i], (UINT32)j, &SizeOfInfo, &info);
			Print(L"mode %03d width %d height %d format %x%s\n", (UINT32)j, info->HorizontalResolution, info->VerticalResolution, info->PixelFormat, j == nativeMode ? "(current)" : "");
		}
	}

	//CpuBreakpoint();

	// Set Video Mode and Get the Framebuffer
	UINT32 mode = 4;

	Status = gop[mode]->SetMode(gop[mode], mode);

	if (EFI_ERROR(Status))
	{
		Print(L"Unable to set mode %03d\n", mode);
	}
	else
	{
		// get framebuffer
		Print(L"Framebuffer address %x size %d, width %d height %d pixelsperline %d\n",
			  gop[mode]->Mode->FrameBufferBase,
			  gop[mode]->Mode->FrameBufferSize,
			  gop[mode]->Mode->Info->HorizontalResolution,
			  gop[mode]->Mode->Info->VerticalResolution,
			  gop[mode]->Mode->Info->PixelsPerScanLine);
	}

	//("For test proposes:\n");
	//printf("System info:\n");
	//printf("System Table: %p \n", gST);
	//printf("Boot Services: %p \n", gBS);
	//printf("Runtime Services: %p \n", gRT);
	//printf("________________________________________\n\n");

	// Получить протокол по дескриптору

	Print(L"Hello diploma!\n");

	//
	UINTN EventIndex;

	Print(L"Press any Key to coninue:\n");
	gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);

	//pGPT = AllocateZeroPool(BlockIo->Media->BlockSize);

	//gBS->LocateHandle(EFI_PROTOCOL, EFI_GRAPHICS_OUTPUT_PROTOCOL, NULL, sizeof(buf), buf);

	return EFI_SUCCESS;
}
