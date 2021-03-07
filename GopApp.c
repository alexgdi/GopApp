
#include "Header.h"

//
#include <Library/BmpSupportLib.h>

//
#include <Library/ShellLib.h>

#include "Colors.h"
#include "Rectangle.h"

// Очистить экран
EFI_STATUS EFIAPI ClearScreen()
{
	gST->ConOut->ClearScreen(gST->ConOut);
}

EFI_STATUS EFIAPI SystemInfo()
{
	Print(L"For test proposes:\n");
	Print(L"System info:\n");
	Print(L"System Table: 0x%p \n", gST);
	Print(L"Boot Services: 0x%p \n", gBS);
	Print(L"Runtime Services: 0x%p \n\n", gRT);
}

EFI_STATUS EFIAPI GopInitialize()
{
	EFI_STATUS Status;

	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN SizeOfInfo, numModes, nativeMode;

	// First, try to open GOP on the Console Out handle. If that fails, try a global database search.
	Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&gGop);
	if (EFI_ERROR(Status))
	{
		Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&gGop);
		if (EFI_ERROR(Status))
		{
			Print(L"Error: Could not find a GOP instance!\n");
			Status = EFI_NOT_FOUND;
			return EFI_ERROR(-1);
		}
	}

	Status = gGop->QueryMode(gGop, gGop->Mode == NULL ? 0 : gGop->Mode->Mode, &SizeOfInfo, &info);

	if (EFI_ERROR(Status))
	{
		Print(L"Unable to get native mode\n");
	}
	else
	{
		nativeMode = gGop->Mode->Mode;
		numModes = gGop->Mode->MaxMode;
		Print(L"NumModes = %d NativeMode = %d\n", numModes, nativeMode);
	}

	for (UINTN j = 0; j < numModes; j++)
	{
		Status = gGop->QueryMode(gGop, (UINT32)j, &SizeOfInfo, &info);
		Print(L"Mode: %d Resolution: %dx%d PixelFormat: %x PixelPerScanLine: %d %s\n", (UINT32)j, info->HorizontalResolution, info->VerticalResolution,
			  info->PixelFormat, info->PixelsPerScanLine, j == nativeMode ? L"(current)" : L"");
	}

	// Устанавливаем видео режим и получаем Framebuffer
	UINT32 mode = 4;

	Status = gGop->SetMode(gGop, mode);

	if (EFI_ERROR(Status))
	{
		Print(L"Unable to set mode %03d\n", mode);
	}
	else
	{
		// get framebuffer
		Print(L"Framebuffer address %x size %d, Resolution: %dx%d PixelsPerLine: %d MaxMode: %d\n", gGop->Mode->FrameBufferBase,
			  gGop->Mode->FrameBufferSize, gGop->Mode->Info->HorizontalResolution, gGop->Mode->Info->VerticalResolution,
			  gGop->Mode->Info->PixelsPerScanLine, gGop->Mode->MaxMode);
	}

	// Чистим память за собой
	// if (handles != NULL)
	// 	FreePool(handles);

	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI DrawBmpImage(CONST CHAR16 *BmpFilePath)
{
	EFI_STATUS Status;
	CONST CHAR16 *BmpFullFilePath;
	VOID *BmpFileData;
	VOID *OriginalVideoBufferData;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
	EFI_FILE_INFO *BmpFileInfo;
	BOOLEAN CursorModified;
	BOOLEAN CursorVisible;
	SHELL_FILE_HANDLE BmpFileHandle;
	UINTN OriginalVideoBltBufferSize;
	UINTN BmpFileSize;
	UINTN BltSize;
	UINTN ImageWidth;
	UINTN ImageHeight;
	INTN ImageDestinationX;
	INTN ImageDestinationY;
	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;

	EFI_INPUT_KEY Key;
	UINTN EventIndex;

	CursorModified = FALSE;
	BmpFileData = NULL;
	OriginalVideoBufferData = NULL;

	// Open the BMP file path requested
	BmpFullFilePath = ShellFindFilePath(BmpFilePath);
	if (BmpFullFilePath == NULL)
	{
		Print(L"Error: The BMP file path %s could not be found\n", BmpFilePath);
		Status = EFI_NOT_FOUND;
		goto Done;
	}

	Status = ShellIsFile(BmpFullFilePath);
	if (EFI_ERROR(Status))
	{
		Print(L"Error: The BMP file path %s is invalid\n", BmpFilePath);
		Status = EFI_NOT_FOUND;
		goto Done;
	}

	Status = ShellOpenFileByName(BmpFullFilePath, &BmpFileHandle, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status))
	{
		Print(L"Error: Could not read the BMP file %s\n", BmpFullFilePath);
		Status = EFI_LOAD_ERROR;
		goto Done;
	}

	BmpFileInfo = ShellGetFileInfo(BmpFileHandle);
	if (BmpFileInfo == NULL)
	{
		Print(L"Error: Failed to get file info for the BMP file %s\n", BmpFullFilePath);
		Status = EFI_LOAD_ERROR;
		goto Done;
	}

	BmpFileSize = (UINTN)BmpFileInfo->FileSize;

	//
	BmpFileData = AllocateZeroPool(BmpFileSize);

	if (BmpFileData == NULL)
	{
		Print(L"Error: Insufficient memory available to load BMP file.\n  BMP file name: %s\n  BMP file size: %s\n", BmpFullFilePath, BmpFileSize);
		Status = EFI_OUT_OF_RESOURCES;
		goto Done;
	}

	//
	Status = ShellReadFile(BmpFileHandle, &BmpFileSize, BmpFileData);

	// Закрываем файл
	ShellCloseFile(&BmpFileHandle);
	BmpFileHandle = NULL;
	if (EFI_ERROR(Status))
	{
		Print(L"Error: Could not read BMP file %s\n", BmpFullFilePath);
		Status = EFI_VOLUME_CORRUPTED;
		goto Done;
	}

	//
	HorizontalResolution = gGop->Mode->Info->HorizontalResolution;
	VerticalResolution = gGop->Mode->Info->VerticalResolution;

	// Выключить курсор
	if (gST->ConOut != NULL)
	{
		CursorModified = TRUE;
		CursorVisible = gST->ConOut->Mode->CursorVisible;
		gST->ConOut->EnableCursor(gST->ConOut, FALSE);
	}

	// Translate the GOP image buffer to a BLT buffer
	Blt = NULL;
	ImageWidth = 0;
	ImageHeight = 0;
	Status = TranslateBmpToGopBlt(BmpFileData, BmpFileSize, &Blt, &BltSize, &ImageHeight, &ImageWidth);

	if (EFI_ERROR(Status))
	{
		Print(L"Error: An error occurred translating the BMP to a GOP BLT - %r.\n", Status);
		goto Done;
	}

	Print(L"Image information:\n");
	Print(L"File name: %s\n  File size: %d bytes\n", BmpFullFilePath, BmpFileSize);
	Print(L"Dimensions: %d x %d.\n", ImageWidth, ImageHeight);

	if (ImageWidth > HorizontalResolution)
	{
		Print(L"Error: The image width (%d px) is too wide for the horizontal resolution (%d px).\n", ImageWidth, HorizontalResolution);
		Status = EFI_ABORTED;
		goto Done;
	}
	if (ImageHeight > VerticalResolution)
	{
		Print(L"Error: The image height (%d px) is too tall for the vertical resolution (%d px).\n", ImageHeight, VerticalResolution);
		Status = EFI_ABORTED;
		goto Done;
	}

	// Backup the current buffer in the area that will be covered by the image
	OriginalVideoBltBufferSize = ImageWidth * ImageHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

	OriginalVideoBufferData = AllocateZeroPool(OriginalVideoBltBufferSize);
	if (OriginalVideoBufferData == NULL)
	{
		Print(L"Error: Insufficient memory available to allocate a BLT buffer of size 0x%x\n", OriginalVideoBltBufferSize);
		Status = EFI_OUT_OF_RESOURCES;
		goto Done;
	}

	ImageDestinationX = (HorizontalResolution - ImageWidth) / 2;
	ImageDestinationY = (VerticalResolution - ImageHeight) / 2;

	if (ImageDestinationX < 0 || ImageDestinationY < 0)
	{
		Print(L"Error: The image size and/or orientation are invalid for this display.\n");
		Status = EFI_ABORTED;
		goto Done;
	}

	Status = gGop->Blt(gGop, OriginalVideoBufferData, EfiBltVideoToBltBuffer, (UINTN)ImageDestinationX, (UINTN)ImageDestinationY,
					   0, 0, ImageWidth, ImageHeight, ImageWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

	if (EFI_ERROR(Status))
	{
		Print(L"Error: An error occurred reading from the video frame buffer!\n");
		Status = EFI_DEVICE_ERROR;
		goto Done;
	}

	// Output the BMP image
	Status = gGop->Blt(gGop, Blt, EfiBltBufferToVideo, 0, 0, (UINTN)ImageDestinationX, (UINTN)ImageDestinationY, ImageWidth,
					   ImageHeight, ImageWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

	if (EFI_ERROR(Status))
	{
		Print(L"Error: An error occurred writing to the video frame buffer!\n");
		Status = EFI_DEVICE_ERROR;
		goto Done;
	}

	// Stop showing the image when a key is pressed
	while (TRUE)
	{
		Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
		if (!EFI_ERROR(Status))
			break;

		if (Status != EFI_NOT_READY)
			continue;

		// Wait for another key press
		gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
	}

	// Restore the original BLT buffer from the image area
	Status = gGop->Blt(gGop, OriginalVideoBufferData, EfiBltBufferToVideo, 0, 0, (UINTN)ImageDestinationX, (UINTN)ImageDestinationY,
					   ImageWidth, ImageHeight, ImageWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

	if (EFI_ERROR(Status))
	{
		Print(L"Error: An error occurred writing to the video frame buffer!\n");
		Status = EFI_DEVICE_ERROR;
	}

Done:

	// Вернуть курсору предыдущее состояние
	if (CursorModified)
	{
		gST->ConOut->EnableCursor(gST->ConOut, CursorVisible);
	}

	// Очистка памяти
	if (Blt != NULL)
	{
		FreePool(Blt);
	}
	if (BmpFileData != NULL)
	{
		FreePool(BmpFileData);
	}
	if (OriginalVideoBufferData != NULL)
	{
		FreePool(OriginalVideoBufferData);
	}

	return Status;
}

#define MAX_NUMBER_OF_ARGS 1

STATIC CONST SHELL_PARAM_ITEM mParamList[] = {
	{L"-?", TypeFlag},	// ? - Help
	{L"-h", TypeFlag},	// h - Help
	{L"-i", TypeValue}, // i - Input file path
	{NULL, TypeMax},
};

EFI_STATUS ParseCommandLine(OUT CONST CHAR16 **BmpFilePath)
{
	EFI_STATUS Status;
	LIST_ENTRY *Package;
	CHAR16 *ProblemParam;
	CONST CHAR16 *LocalBmpFilePath;

	Package = NULL;
	ProblemParam = NULL;

	if (BmpFilePath == NULL)
	{
		return EFI_INVALID_PARAMETER;
	}

	Status = ShellCommandLineParse(mParamList, &Package, &ProblemParam, FALSE);
	if (EFI_ERROR(Status))
	{
		if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL)
		{
			Print(L"Error: Unknown parameter input: %s\n", ProblemParam);
			goto Done;
		}
	}

	if (ShellCommandLineGetCount(Package) > MAX_NUMBER_OF_ARGS)
	{
		Print(L"Error: Too many arguments. Maximum of %d expected.\n", MAX_NUMBER_OF_ARGS);
		Status = EFI_INVALID_PARAMETER;
		goto Done;
	}

	if (ShellCommandLineGetFlag(Package, L"-?") || ShellCommandLineGetFlag(Package, L"-h"))
	{
		//PrintUsage();
		Status = EFI_SUCCESS;
		*BmpFilePath = NULL;
		goto Done;
	}

	LocalBmpFilePath = ShellCommandLineGetValue(Package, L"-i");
	if (LocalBmpFilePath == NULL)
	{
		Print(L"Error: An input BMP file must be specified.\n");
		Status = EFI_INVALID_PARAMETER;
		goto Done;
	}

	*BmpFilePath = LocalBmpFilePath;

Done:
	if (ProblemParam != NULL)
	{
		FreePool(ProblemParam);
	}

	return Status;
}

// The Entry Point for Application. The user code starts with this function
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN EventIndex;

	//CONST CHAR16 *BmpFilePath = L"test.bmp";

	// Ожидание нажатия клавиши
	gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);

	CpuBreakpoint();

	Print(L"Hello diploma!\n\n");

	GopInitialize();

	// Заливка сплошным цветом
	//EFI_GRAPHICS_OUTPUT_BLT_PIXEL p = MS_GRAPHICS_DARK_GRAY_1_COLOR;

	//DrawRectangle(0, 0, 100, 100, &p);

	//EFI_GRAPHICS_OUTPUT_BLT_PIXEL p1 = MS_GRAPHICS_GREEN_COLOR;

	//DrawRectangle(200, 200, 100, 100, &p1);

	//EFI_GRAPHICS_OUTPUT_BLT_PIXEL p2 = MS_GRAPHICS_SKY_BLUE_COLOR;

	//DrawRectangle(700, 200, 100, 100, &p2);

	//DrawBmpImage(BmpFilePath);

	UI_STYLE_INFO si;
	si.Border.BorderWidth = 0;

	si.FillType = FILL_SOLID;
	si.FillTypeInfo.SolidFill.FillColor = COLOR_ORANGE;

	si.IconInfo.Width = 0;
	si.IconInfo.Height = 0;
	si.IconInfo.PixelData = NULL;

	POINT ul = {100, 100};

	UI_RECTANGLE *rect = new_UI_RECTANGLE(&ul, (UINT8 *)((UINTN)gGop->Mode->FrameBufferBase), gGop->Mode->Info->PixelsPerScanLine, 300, 300, &si);
	DrawRect(rect);
	//delete_UI_RECTANGLE(rect);

	DEBUG((DEBUG_INFO, "INFO [GOP]: GopApp exit - code=%r\n", Status));

	return EFI_SUCCESS;
}
