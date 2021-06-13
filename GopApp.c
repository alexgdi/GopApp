
#include "Header.h"

//
#include <Library/BmpSupportLib.h>

//
#include <Library/ShellLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileInfo.h>

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

	// Используем дескриптор ConsoleOutHandle для получения протокола GOP,
	// в случае неудачи выполняем поиск в глобальной базе протоколов
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
		Print(L"Unable to get native mode\n");
	else
	{
		nativeMode = gGop->Mode->Mode;
		numModes = gGop->Mode->MaxMode;
		Print(L"NumModes = %d NativeMode = %d\n", numModes, nativeMode);
	}

	for (UINTN j = 0; j < numModes; j++)
	{
		Status = gGop->QueryMode(gGop, (UINT32)j, &SizeOfInfo, &info);
		Print(L"Mode: %d Resolution: %dx%d PixelFormat: %x PixelPerScanLine: %d %s\n", (UINT32)j,
			  info->HorizontalResolution, info->VerticalResolution,
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
	fontDisplayInfo->FontInfo.FontSize = 19;

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

EFI_STATUS DrawBmpImage(CONST CHAR16 *BmpFilePath)
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

EFI_STATUS VolumeInfo(EFI_HANDLE handle, INTN i)
{
	EFI_STATUS Status;

	// ФАЙЛОВАЯ СИСТЕМА
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *gSfsp;

	EFI_FILE_PROTOCOL *gRoot;
	UINTN BufferSize;
	EFI_FILE_SYSTEM_INFO *FsInfo;

	//EFI_FILE_SYSTEM_VOLUME_LABEL *VolumeLabel;

	// Получить дескрипторы этого протокола
	Status = gBS->HandleProtocol(handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&gSfsp);

	if (EFI_ERROR(Status))
	{
		Print(L"HandleProtocol %d error!\n", i);
		return Status;
	}

	// Получение корневого каталога
	Status = gSfsp->OpenVolume(gSfsp, &gRoot);

	if (EFI_ERROR(Status))
	{
		Print(L"Error: OpenVolume error!\n");
		Status = EFI_NOT_FOUND;
		return Status;
	}

	/*// EFI_FILE_SYSTEM_VOLUME_LABEL - структура переменной длины
	BufferSize = sizeof(EFI_FILE_SYSTEM_VOLUME_LABEL) + sizeof(CHAR16) * 512;
	Status = gBS->AllocatePool(EfiBootServicesCode, BufferSize, (VOID **)&VolumeLabel);

	Status = gRoot->GetInfo(gRoot, &gEfiFileSystemVolumeLabelInfoIdGuid, &BufferSize, VolumeLabel);

	// Если размера передаваемого буфера не хватило, выделить буфер необходимого размера.
	if (Status == EFI_BUFFER_TOO_SMALL)
	{
		Status = gBS->FreePool(VolumeLabel);
		Status = gBS->AllocatePool(EfiBootServicesCode, BufferSize, (VOID **)&VolumeLabel);
		Status = gRoot->GetInfo(gRoot, &gEfiFileSystemVolumeLabelInfoIdGuid, &BufferSize, VolumeLabel);
	}

	// Вывести информацию о файловой системе
	Print(L"Volume: %d %s\n", i, VolumeLabel->VolumeLabel);

	Status = gBS->FreePool(VolumeLabel);*/

	// EFI_FILE_SYSTEM_INFO - структура переменной длины
	BufferSize = sizeof(EFI_FILE_SYSTEM_INFO) + sizeof(CHAR16) * 512;
	Status = gBS->AllocatePool(EfiBootServicesCode, BufferSize, (VOID **)&FsInfo);

	Status = gRoot->GetInfo(gRoot, &gEfiFileSystemInfoGuid, &BufferSize, FsInfo);

	// Если размера передаваемого буфера не хватило, выделить буфер необходимого размера.
	if (Status == EFI_BUFFER_TOO_SMALL)
	{
		Status = gBS->FreePool(FsInfo);
		Status = gBS->AllocatePool(EfiBootServicesCode, BufferSize, (VOID **)&FsInfo);
		Status = gRoot->GetInfo(gRoot, &gEfiFileSystemInfoGuid, &BufferSize, FsInfo);
	}

	// Вывести информацию о файловой системе
	Print(L"Volume %d: %s bs: %u bytes size: %lu MB vlsize: %lu MB free: %lu MB RO:%s\n", i, FsInfo->VolumeLabel, FsInfo->BlockSize, FsInfo->Size / 1024 / 1024, FsInfo->VolumeSize / 1024 / 1024, FsInfo->FreeSpace / 1024 / 1024, FsInfo->ReadOnly ? L"YES" : L"NO");

	Status = gBS->FreePool(FsInfo);
}

EFI_STATUS ListVolumes()
{
	EFI_STATUS Status;

	EFI_HANDLE *handles;
	UINTN handles_count = 10;

	// Выделение памяти
	handles = AllocateZeroPool(sizeof(EFI_HANDLE) * handles_count);

	// Получение дескрипторов всех томов
	Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &handles_count, &handles);

	// if (...)
	// {

	// }

	// Вывод кол-ва обнаруженных томов
	Print(L"gLocateHandleBuffer: %d  HandlesCount: %d\n", Status, handles_count);

	// Вывод
	for (INTN i = 0; i < handles_count; i++)
	{
		VolumeInfo(handles[i], i);
	}
}

EFI_STATUS DrawBmpImage1(EFI_HANDLE handle, CONST CHAR16 *FilePath)
{
	EFI_STATUS Status;

	// Файл
	VOID *FileData;
	UINTN FileSize;
	EFI_FILE_INFO *FileInfo;

	UINTN BufferSize;
	UINTN ReadSize;

	VOID *OriginalVideoBufferData;

	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;

	BOOLEAN CursorModified;
	BOOLEAN CursorVisible;

	UINTN OriginalVideoBltBufferSize;
	UINTN BltSize;
	UINTN ImageWidth;
	UINTN ImageHeight;
	INTN ImageDestinationX;
	INTN ImageDestinationY;
	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;

	EFI_INPUT_KEY Key;
	UINTN EventIndex;

	SHELL_FILE_HANDLE BmpFileHandle;

	CursorModified = FALSE;
	FileData = NULL;
	OriginalVideoBufferData = NULL;

	// ФАЙЛОВАЯ СИСТЕМА
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *gSfsp;

	EFI_FILE_PROTOCOL *gRoot;
	EFI_FILE_PROTOCOL *File;

	// Получить дескрипторы этого протокола
	Status = gBS->HandleProtocol(handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&gSfsp);

	if (EFI_ERROR(Status))
	{
		Print(L"HandleProtocol %d error!\n");
	}

	// Получение корневого каталога
	Status = gSfsp->OpenVolume(gSfsp, &gRoot);

	if (EFI_ERROR(Status))
	{
		Print(L"Error: OpenVolume error!\n");
		Status = EFI_NOT_FOUND;
	}

	// Открыть требуемый файл
	Status = gRoot->Open(gRoot, &File, FilePath, EFI_FILE_MODE_READ, NULL);

	// Открыть файл
	// Считать до конца
	// TranslateBmpToGop
	// Вывести GOP на экран

	// Получение размера файла GetFileSize

	// EFI_FILE_INFO - структура переменной длины
	BufferSize = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 512;
	Status = gBS->AllocatePool(EfiBootServicesCode, BufferSize, (VOID **)&FileInfo);

	ReadSize = BufferSize;

	// Получаем информацию о файле
	Status = File->GetInfo(File, &gEfiFileInfoGuid, &ReadSize, FileInfo);

	// Если размера передаваемого буфера не хватило, выделить буфер необходимого размера.
	if (Status == EFI_BUFFER_TOO_SMALL)
	{
		BufferSize = ReadSize;
		Status = gBS->FreePool(FileInfo);
		Status = gBS->AllocatePool(EfiBootServicesCode, ReadSize, (VOID **)&FileInfo);
		Status = File->GetInfo(File, &gEfiFileInfoGuid, &ReadSize, FileInfo);
	}

	FileSize = FileInfo->FileSize;

	// Выделяем память
	FileData = AllocateZeroPool(FileSize);

	if (FileData == NULL)
	{
		Print(L"Error: Insufficient memory available to load file.\n  file name: %s\n  BMP file size: %s\n", FilePath, FileSize);
		Status = EFI_OUT_OF_RESOURCES;
		return Status;
	}

	ReadSize = FileSize;

	// Чтение файла
	Status = File->Read(File, &ReadSize, FileData);

	// Если размера передаваемого буфера не хватило, выделить буфер необходимого размера.
	if (Status == EFI_BUFFER_TOO_SMALL)
	{
		FileSize = ReadSize;
		Status = gBS->FreePool(FileData);
		Status = gBS->AllocatePool(EfiBootServicesCode, FileSize, (VOID **)&FileData);
		Status = File->Read(File, &ReadSize, FileData);
	}

	// Закрываем файл
	File->Close(File);

	// Получение разрешения экрана
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
	Status = TranslateBmpToGopBlt(FileData, FileSize, &Blt, &BltSize, &ImageHeight, &ImageWidth);

	if (EFI_ERROR(Status))
	{
		Print(L"Error: An error occurred translating the BMP to a GOP BLT - %r.\n", Status);
		goto Done;
	}

	Print(L"Image information:\n");
	Print(L"File name: %s\n  File size: %d bytes\n", FilePath, FileSize);
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

	//Очистка памяти
	if (Blt != NULL)
	{
		FreePool(Blt);
	}

	if (FileData != NULL)
	{
		FreePool(FileData);
	}

	if (OriginalVideoBufferData != NULL)
	{
		FreePool(OriginalVideoBufferData);
	}

	// Освобождение памяти
	FreePool(FileInfo);

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

	// Получение имени текущего шрифта и его параметров
	Status = gHiiFontProtocol->GetFontInfo(gHiiFontProtocol, NULL, NULL, &FontInfo, NULL);

	Print(L"FontName: %s FontSize: %d FontStyle: %x\n", FontInfo->FontInfo.FontName,
		  FontInfo->FontInfo.FontSize, FontInfo->FontInfo.FontStyle);
	Print(L"Background color RGB(%d, %d, %d)\n", FontInfo->BackgroundColor.Red,
		  FontInfo->BackgroundColor.Green, FontInfo->BackgroundColor.Blue);
	Print(L"Foreground color RGB(%d, %d, %d)\n", FontInfo->ForegroundColor.Red,
		  FontInfo->ForegroundColor.Green, FontInfo->ForegroundColor.Blue);
	Print(L"FontInfoMask 0x%08x)\n", FontInfo->FontInfoMask);

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

void TimeOutput(EFI_TIME time)
{
	Print(L"%d.%d.%d %d:%d:%d ", time.Day, time.Month, time.Year, time.Hour, time.Minute, time.Second);
}

EFI_STATUS
ListDirectory(EFI_FILE_PROTOCOL *Directory)
{
	EFI_STATUS Status = 0;
	UINTN EventIndex;

	UINTN BufferSize;
	UINTN ReadSize;
	EFI_FILE_INFO *FileInfo;

	// EFI_FILE_INFO - структура переменной длины
	BufferSize = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 512;
	Status = gBS->AllocatePool(EfiBootServicesCode, BufferSize, (VOID **)&FileInfo);

	while (1)
	{
		ReadSize = BufferSize;

		// Выполняем чтение записи каталога
		Status = Directory->Read(Directory, &ReadSize, FileInfo);

		// Если размера передаваемого буфера не хватило, выделить буфер необходимого размера.
		if (Status == EFI_BUFFER_TOO_SMALL)
		{
			BufferSize = ReadSize;
			BREAK_ERR(Status = gBS->FreePool(FileInfo));
			BREAK_ERR(Status = gBS->AllocatePool(EfiBootServicesCode, BufferSize, (VOID **)&FileInfo));
			BREAK_ERR(Status = Directory->Read(Directory, &ReadSize, FileInfo));
		}

		// Больше нет записей в каталоге, выход.
		if (ReadSize == 0)
			break;

		BREAK_ERR(Status);

		// Вывод информации о записи каталога
		Print(L"%s Size: %d FileSize: %d Physical Size: %d\n", FileInfo->FileName, FileInfo->Size, FileInfo->FileSize, FileInfo->PhysicalSize);
		Print(L"C: ");
		TimeOutput(FileInfo->CreateTime);
		Print(L" A: ");
		TimeOutput(FileInfo->CreateTime);
		Print(L" M: ");
		TimeOutput(FileInfo->CreateTime);
		Print(L"\n");
		Print(L"Attr: %s%s%s%s%s%s\n", FileInfo->Attribute & EFI_FILE_READ_ONLY ? L"RO " : L"", FileInfo->Attribute & EFI_FILE_HIDDEN ? L"H " : L"", FileInfo->Attribute & EFI_FILE_SYSTEM ? L"S " : "", FileInfo->Attribute & EFI_FILE_RESERVED ? L"R " : L"", FileInfo->Attribute & EFI_FILE_DIRECTORY ? L"D " : L"", FileInfo->Attribute & EFI_FILE_ARCHIVE ? L"A " : L"");

		// Ожидание нажатия клавиши
		gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
	}

	Status = gBS->FreePool(FileInfo);
	return 0;
}

EFI_STATUS PointerInit()
{
	EFI_STATUS Status;

	// Получаем экземпляр протокола Simple Pointer Protocol
	Status = gBS->LocateProtocol(&gEfiSimplePointerProtocolGuid, NULL, (VOID **)&gSpp);

	if (EFI_ERROR(Status))
	{
		Print(L"Error: Could not find a Simple Pointer Protocol instance!\n");
		Status = EFI_NOT_FOUND;
		return EFI_UNSUPPORTED;
	}

	UINTN handles_count = 10;

	EFI_HANDLE *handles;

	handles = AllocateZeroPool(sizeof(EFI_HANDLE) * handles_count);

	Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimplePointerProtocolGuid, NULL, &handles_count, &handles);

	Print(L"gLocateHandleBuffer=%d num=%d\n", Status, handles_count);

	// Получить протокол по дескриптору
	Status = gBS->HandleProtocol(handles[1], &gEfiSimplePointerProtocolGuid, (void **)&gSpp);

	if (EFI_ERROR(Status))
	{
		Print(L"HandleProtocol error!\n");
	}

	return EFI_SUCCESS;
}

// Не работает
EFI_STATUS LoadDriver(EFI_HANDLE handle, CONST CHAR16 *DriverPath)
{
	EFI_STATUS Status;

	EFI_HANDLE DriverHandle;
	EFI_DEVICE_PATH_PROTOCOL *FileDevPath = NULL;

	//FileDevPath = FileDevicePath(handle, DriverPath);

	if (!IsDevicePathValid(FileDevPath, 0))
	{
		Print(L"Error: Invalid Device path!\n");
		Status = EFI_NOT_FOUND;
		return EFI_ERROR(-1);
	}

	if (FileDevPath == NULL)
	{
		Print(L"Error: Could not get driver device path!\n");
		Status = EFI_NOT_FOUND;
		return EFI_ERROR(-1);
	}

	Status = gBS->LoadImage(FALSE, gImageHandle, FileDevPath, NULL, NULL, &DriverHandle);

	if (EFI_ERROR(Status))
	{
		Print(L"Error: Could not load driver!\n");
		Status = EFI_NOT_FOUND;
		return EFI_ERROR(-1);
	}

	return EFI_SUCCESS;
}

// The Entry Point for Application. The user code starts with this function
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status = EFI_SUCCESS;

	EFI_GRAPHICS_OUTPUT_BLT_PIXEL p = {.Red = 255, .Green = 0, .Blue = 0};
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL p2 = {.Red = 0, .Green = 255, .Blue = 0};

	UINTN EventIndex;

	//Print(L"Hello diploma!\n");

	// Ожидание нажатия клавиши
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
	//GetGopInfo();

	// Ожидание нажатия клавиши
	//gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);

	// Устанавливаем видеорежим 4
	//SetGopMode(4);

	//
	DemoDrawRectangles();

	// Иннициализация HII Font Protocol
	Status = HiiFontInit();

	if (Status != EFI_SUCCESS)
	{
		Print(L"Hii load error!\n");
		return EFI_ERROR(Status);
	}

	// Информация о текущем шрифте
	//GetFontInfo();

	//Status = DrawText(L"HelloDONNTU!\0", 100, 100, p, p2);
	//Status = DrawText(L"HelloDONNTU!\0", 10, 10, p, p2);
	//Status = DrawText(L"HelloDONNTU!\0", 10, 500, p, p2);

	if (Status != EFI_SUCCESS)
	{
		Print(L"Draw text error!\n");
		return EFI_ERROR(Status);
	}

	//EFI_HANDLE *handles;
	//UINTN handles_count = 10;

	// Выделение памяти
	//handles = AllocateZeroPool(sizeof(EFI_HANDLE) * handles_count);

	// Получение дескрипторов всех томов
	//Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &handles_count, &handles);

	//Print(L"HandlesCount: %d\n", handles_count);

	//Вывод изображения
	//CONST CHAR16 *BmpFilePath = L"test.bmp";
	//DrawBmpImage1(handles[2], BmpFilePath);

	//Print(L"Hello!\n");

	// Загрузка драйвера
	//CONST CHAR16 FilePath = L"\\ntfs_x64.efi";
	//LoadDriver(handles[0], FilePath);

	// ФАЙЛОВАЯ СИСТЕМА
	// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *gSfsp;

	// Status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID **)&gSfsp);

	// if (EFI_ERROR(Status))
	// {
	// 	Print(L"Error: Could not find a GOP instance!\n");
	// 	Status = EFI_NOT_FOUND;
	// 	return EFI_ERROR(-1);
	// }

	// EFI_FILE_PROTOCOL *gFile1;

	// EFI_FILE_INFO fileinfo;
	// unsigned fileinfosize = sizeof(EFI_FILE_INFO);

	// EFI_FILE_PROTOCOL *gRoot;

	// Status = gSfsp->OpenVolume(gSfsp, &gRoot);

	// if (EFI_ERROR(Status))
	// {
	// 	Print(L"Error: OpenVolume error!\n");
	// 	Status = EFI_NOT_FOUND;
	// 	return EFI_ERROR(-1);
	// }

	// ListDirectory(gRoot);

	// Указатель Мышь
	// PointerInit();

	// Print(L"Pointer Mode: %d %d %d %d %d\n", gSpp->Mode->ResolutionX, gSpp->Mode->ResolutionY, gSpp->Mode->ResolutionZ, gSpp->Mode->LeftButton, gSpp->Mode->RightButton);

	DEBUG((DEBUG_INFO, "INFO [GOP]: GopApp exit - code=%r\n", Status));

	return EFI_SUCCESS;
}
