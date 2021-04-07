
#include "Header.h"

//
#include <Library/BmpSupportLib.h>

//
#include <Library/ShellLib.h>

//
#include "Colors.h"
//#include "Rectangle.h"

// Очистить экран
EFI_STATUS ClearScreen()
{
	EFI_STATUS Status;

	Status = gST->ConOut->ClearScreen(gST->ConOut);

	return Status;
}

EFI_STATUS SystemInfo()
{
	Print(L"For test proposes:\n");
	Print(L"System info:\n");
	Print(L"System Table: 0x%p \n", gST);
	Print(L"Boot Services: 0x%p \n", gBS);
	Print(L"Runtime Services: 0x%p \n\n", gRT);
}

EFI_STATUS GopInit()
{
	EFI_STATUS Status;

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

	return EFI_SUCCESS;
}

EFI_STATUS GetGopInfo()
{
	EFI_STATUS Status;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN SizeOfInfo, numModes, nativeMode;

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

	return EFI_SUCCESS;
}

// Режим 4
EFI_STATUS SetGopMode(UINT32 mode)
{
	EFI_STATUS Status;

	// Устанавливаем видео режим и получаем Framebuffer
	Status = gGop->SetMode(gGop, mode);

	if (EFI_ERROR(Status))
	{
		Print(L"Unable to set mode %03d\n", mode);
	}
	else
	{
		// Получаем адрес фреймбуффера
		Print(L"Framebuffer address %x size %d, Resolution: %dx%d PixelsPerLine: %d MaxMode: %d\n", gGop->Mode->FrameBufferBase,
			  gGop->Mode->FrameBufferSize, gGop->Mode->Info->HorizontalResolution, gGop->Mode->Info->VerticalResolution,
			  gGop->Mode->Info->PixelsPerScanLine, gGop->Mode->MaxMode);
	}
}

EFI_STATUS HiiFontInit()
{
	EFI_STATUS Status;

	// Получаем ссылку на экземпляр протокола EFI_HII_FONT_PROTOCOL
	Status = gBS->LocateProtocol(&gEfiHiiFontProtocolGuid, NULL, (VOID **)&gHiiFontProtocol);
	if (EFI_ERROR(Status))
	{
		Print(L"Error: Could not find a Hii Font Protocol instance!\n");
		Status = EFI_NOT_FOUND;
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

// Вывод текста на экран
EFI_STATUS DrawText(CONST CHAR16 *string, UINTN x, UINTN y, EFI_GRAPHICS_OUTPUT_BLT_PIXEL foregroundcolor, EFI_GRAPHICS_OUTPUT_BLT_PIXEL backgroundcolor)
{
	EFI_STATUS Status;

	//
	EFI_IMAGE_OUTPUT Blt;
	EFI_IMAGE_OUTPUT *pBlt = &Blt;

	//
	EFI_FONT_DISPLAY_INFO *fontDisplayInfo;

	//
	//fontDisplayInfo = (EFI_FONT_DISPLAY_INFO *)AllocateZeroPool(sizeof(EFI_FONT_DISPLAY_INFO) + StrLen((const CHAR16 *)L"A") * 2 + 2);
	fontDisplayInfo = AllocateZeroPool(sizeof(EFI_FONT_DISPLAY_INFO));

	// Выбор цвета шрифта
	fontDisplayInfo->ForegroundColor = foregroundcolor;

	// Выбор цвета фона
	fontDisplayInfo->BackgroundColor = backgroundcolor;

	// Выбор шрифта
	//fontDisplayInfo->FontInfo.FontName = "sysdefault";

	// Стиль шрифта
	// EFI_HII_FONT_STYLE_NORMAL
	// EFI_HII_FONT_STYLE_BOLD
	// EFI_HII_FONT_STYLE_ITALIC
	// EFI_HII_FONT_STYLE_EMBOSS
	// EFI_HII_FONT_STYLE_OUTLINE
	// EFI_HII_FONT_STYLE_SHADOW
	// EFI_HII_FONT_STYLE_UNDERLINE
	// EFI_HII_FONT_STYLE_DBL_UNDER

	fontDisplayInfo->FontInfo.FontStyle = EFI_HII_FONT_STYLE_NORMAL;

	// Высота ячейки символа в пикселях
	fontDisplayInfo->FontInfo.FontSize = 5;

	//fontDisplayInfo->FontInfoMask = EFI_FONT_INFO_SYS_FORE_COLOR | EFI_FONT_INFO_SYS_BACK_COLOR | EFI_FONT_INFO_RESIZE;
	fontDisplayInfo->FontInfoMask = EFI_FONT_INFO_SYS_SIZE | EFI_FONT_INFO_SYS_FONT;

	//CopyMem(fontDisplayInfo->FontInfo.FontName, (const CHAR16 *)L"FZLKSXSJW", StrLen((const CHAR16 *)L"A") * 2 + 2);

	if (gGop == NULL)
	{
		Print(L"Gop == NULL!\n");
		return EFI_ERROR(-1);
	}

	Blt.Width = (UINT16)gGop->Mode->Info->HorizontalResolution;

	Blt.Height = (UINT16)gGop->Mode->Info->VerticalResolution;

	Blt.Image.Screen = gGop;

	// Вывод строки на экран. Если FontDisplayInfo = NULL, то текст будет выведен шрифтом по умолчанию.
	Status = gHiiFontProtocol->StringToImage(gHiiFontProtocol,
											 EFI_HII_DIRECT_TO_SCREEN | EFI_HII_IGNORE_IF_NO_GLYPH,
											 string, fontDisplayInfo, &pBlt, x, y, 0, 0, 0);

	return Status;
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

EFI_STATUS GetFontInfo()
{
	EFI_STATUS Status;

	//структура, из которой получаем текстовую информацию о шрифте
	EFI_FONT_DISPLAY_INFO *FontInfo;

	// Gолучим текст с именем найденного шрифта
	Status = gHiiFontProtocol->GetFontInfo(gHiiFontProtocol, NULL, NULL, &FontInfo, NULL);

	Print(L"FontName: %s FontSize: %d FontStyle: %x\n", FontInfo->FontInfo.FontName, FontInfo->FontInfo.FontSize, FontInfo->FontInfo.FontStyle);

	Print(L"Background color RGB(%d, %d, %d)\n", FontInfo->BackgroundColor.Red, FontInfo->BackgroundColor.Green, FontInfo->BackgroundColor.Blue);

	Print(L"Foreground color RGB(%d, %d, %d)\n", FontInfo->ForegroundColor.Red, FontInfo->ForegroundColor.Green, FontInfo->ForegroundColor.Blue);

	Print(L"FontInfoMask %x)\n", FontInfo->FontInfoMask);

	//DEBUG((EFI_D_INFO, "GetFontInfo status = %r, current font has '%s' name\n\r", Status, FontInfo->FontInfo.FontName));
	return EFI_SUCCESS;
}

GLOBAL_REMOVE_IF_UNREFERENCED EFI_GRAPHICS_OUTPUT_BLT_PIXEL mEfiColors[16] = {
	{0x00, 0x00, 0x00, 0x00},
	{0x98, 0x00, 0x00, 0x00},
	{0x00, 0x98, 0x00, 0x00},
	{0x98, 0x98, 0x00, 0x00},
	{0x00, 0x00, 0x98, 0x00},
	{0x98, 0x00, 0x98, 0x00},
	{0x00, 0x98, 0x98, 0x00},
	{0x98, 0x98, 0x98, 0x00},
	{0x10, 0x10, 0x10, 0x00},
	{0xff, 0x10, 0x10, 0x00},
	{0x10, 0xff, 0x10, 0x00},
	{0xff, 0xff, 0x10, 0x00},
	{0x10, 0x10, 0xff, 0x00},
	{0xf0, 0x10, 0xff, 0x00},
	{0x10, 0xff, 0xff, 0x00},
	{0xff, 0xff, 0xff, 0x00}};

UINTN TestPrintGraphic(IN UINTN PointX, IN UINTN PointY, IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Foreground,
					   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Background, IN CHAR16 *Buffer, IN UINTN PrintNum)
{
	EFI_STATUS Status;

	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;
	UINT32 ColorDepth;
	UINT32 RefreshRate;

	EFI_HII_FONT_PROTOCOL *HiiFont;
	EFI_IMAGE_OUTPUT *Blt;
	EFI_FONT_DISPLAY_INFO FontInfo;
	EFI_HII_ROW_INFO *RowInfoArray;
	UINTN RowInfoArraySize;

	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Sto;
	EFI_HANDLE ConsoleHandle;
	UINTN Width;
	UINTN Height;
	UINTN Delta;

	HorizontalResolution = 0;
	VerticalResolution = 0;
	Blt = NULL;
	RowInfoArray = NULL;

	ConsoleHandle = gST->ConsoleOutHandle;

	ASSERT(ConsoleHandle != NULL);

	//
	Status = gBS->HandleProtocol(ConsoleHandle, &gEfiSimpleTextOutProtocolGuid, (VOID **)&Sto);

	if (EFI_ERROR(Status))
	{
		goto Error;
	}

	//
	if (gGop != NULL)
	{
		HorizontalResolution = gGop->Mode->Info->HorizontalResolution;
		VerticalResolution = gGop->Mode->Info->VerticalResolution;
	}

	ASSERT((HorizontalResolution != 0) && (VerticalResolution != 0));

	// Получаем HiiFontProtocol
	Status = gBS->LocateProtocol(&gEfiHiiFontProtocolGuid, NULL, (VOID **)&HiiFont);
	if (EFI_ERROR(Status))
	{
		goto Error;
	}

	Blt = (EFI_IMAGE_OUTPUT *)AllocateZeroPool(sizeof(EFI_IMAGE_OUTPUT));
	ASSERT(Blt != NULL);

	Blt->Width = (UINT16)(HorizontalResolution);
	Blt->Height = (UINT16)(VerticalResolution);

	ZeroMem(&FontInfo, sizeof(EFI_FONT_DISPLAY_INFO));

	EFI_FONT_DISPLAY_INFO *ptr = &FontInfo;

	Status = HiiFont->GetFontInfo(HiiFont, NULL, NULL, &ptr, NULL);

	if (EFI_ERROR(Status))
	{
		Print(L"FontInfo Error!\n");
		goto Error;
	}

	//
	if (Foreground != NULL)
	{
		CopyMem(&FontInfo.ForegroundColor, Foreground, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	}
	else
	{
		CopyMem(&FontInfo.ForegroundColor, &mEfiColors[Sto->Mode->Attribute & 0x0f], sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	}

	//
	if (Background != NULL)
	{
		CopyMem(&FontInfo.BackgroundColor, Background, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	}
	else
	{
		CopyMem(&FontInfo.BackgroundColor, &mEfiColors[Sto->Mode->Attribute >> 4], sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	}

	//
	if (gGop != NULL)
	{
		//
		Blt->Image.Screen = gGop;
		Blt->Height = 100;
		Blt->Width = 100;

		Status = HiiFont->StringToImage(HiiFont,
										EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_OUT_FLAG_CLIP | EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y | EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
										Buffer, &FontInfo, &Blt, PointX, PointY, &RowInfoArray, &RowInfoArraySize, NULL);
		if (EFI_ERROR(Status))
		{
			Print(L"String Error!\n");
			goto Error;
		}
	}

	//
	// Calculate the number of actual printed characters
	//
	if (RowInfoArraySize != 0)
	{
		PrintNum = RowInfoArray[0].EndIndex - RowInfoArray[0].StartIndex + 1;
	}
	else
	{
		PrintNum = 0;
	}

	FreePool(RowInfoArray);
	FreePool(Blt);
	return PrintNum;

Error:

	Print(L"Error!");

	if (Blt != NULL)
	{
		FreePool(Blt);
	}
	return 0;
}

// Демонстрация вывода набора прямоугольников
EFI_STATUS DemoDrawRectangles()
{
	// Заливка сплошным цветом
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL p = MS_GRAPHICS_DARK_GRAY_1_COLOR;

	DrawRectangle(0, 0, 100, 100, &p);

	EFI_GRAPHICS_OUTPUT_BLT_PIXEL p1 = MS_GRAPHICS_GREEN_COLOR;

	DrawRectangle(200, 200, 100, 100, &p1);

	EFI_GRAPHICS_OUTPUT_BLT_PIXEL p2 = MS_GRAPHICS_SKY_BLUE_COLOR;

	DrawRectangle(700, 200, 100, 100, &p2);
}

EFI_STATUS DemoDrawText()
{
}

// The Entry Point for Application. The user code starts with this function
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status = EFI_SUCCESS;

	EFI_GRAPHICS_OUTPUT_BLT_PIXEL p = {.Red = 255, .Green = 0, .Blue = 0};
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL p2 = {.Red = 0, .Green = 255, .Blue = 0};

	//Print(L"Hello diploma!\n");

	// Ожидание нажатия клавиши
	// UINTN EventIndex;
	//gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);

	//CpuBreakpoint();

	// Очистка экрана
	Status = ClearScreen();

	// Иннициализация Graphics Output Protocol
	Status = GopInit();

	if (Status != EFI_SUCCESS)
	{
		Print(L"Gop load error!\n");
		return EFI_ERROR(Status);
	}

	// Получаем информацию о GOP
	GetGopInfo();

	// Устанавливаем видеорежим 4
	SetGopMode(4);

	// Иннициализация HII Font Protocol
	Status = HiiFontInit();

	if (Status != EFI_SUCCESS)
	{
		Print(L"Hii load error!\n");
		return EFI_ERROR(Status);
	}

	DemoDrawRectangles();

	// Информация о текущем шрифте
	GetFontInfo();

	//TestPrintGraphic(400, 400, &p, &p2, L"11111111111111111111111111111111", 20);

	//CONST CHAR16 *BmpFilePath = L"test.bmp";
	//DrawBmpImage(BmpFilePath);

	Status = DrawText(L"HelloDONNTU!\0", 100, 100, p, p2);
	Status = DrawText(L"HelloDONNTU!\0", 10, 10, p, p2);
	Status = DrawText(L"HelloDONNTU!\0", 10, 500, p, p2);

	if (Status != EFI_SUCCESS)
	{
		Print(L"Draw text error!\n");
		return EFI_ERROR(Status);
	}

	DEBUG((DEBUG_INFO, "INFO [GOP]: GopApp exit - code=%r\n", Status));

	return EFI_SUCCESS;
}
