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

EFI_HANDLE mImageHandle;

// Protocols.
//
EFI_GRAPHICS_OUTPUT_PROTOCOL *mGop;
EFI_HII_FONT_PROTOCOL *mFont;

// UI Elements.
//
UINT32 mTitleBarWidth, mTitleBarHeight;
UINT32 mMasterFrameWidth, mMasterFrameHeight;
ListBox *mTopMenu;
BOOLEAN mShowFullMenu = FALSE; // By default we won't show the full FrontPage menu (requires validation if there's a system password).

// Master Frame - Form Notifications.
//
UINT32 mCurrentFormIndex;
EFI_EVENT mMasterFrameNotifyEvent;
DISPLAY_ENGINE_SHARED_STATE mDisplayEngineState;
BOOLEAN mTerminateFrontPage = FALSE;
BOOLEAN mResetRequired;
FRONT_PAGE_AUTH_TOKEN_PROTOCOL *mFrontPageAuthTokenProtocol = NULL;
DFCI_AUTHENTICATION_PROTOCOL *mAuthProtocol = NULL;
EFI_HII_CONFIG_ROUTING_PROTOCOL *mHiiConfigRouting;
DFCI_SETTING_ACCESS_PROTOCOL *mSettingAccess;
DFCI_AUTH_TOKEN mAuthToken;

extern EFI_HII_HANDLE gStringPackHandle;
extern EFI_GUID gMsEventMasterFrameNotifyGroupGuid;

//
// Boot video resolution and text mode.
//
UINT32 mBootHorizontalResolution = 0;
UINT32 mBootVerticalResolution = 0;

EFI_FORM_BROWSER2_PROTOCOL *mFormBrowser2;
MS_SIMPLE_WINDOW_MANAGER_PROTOCOL *mSWMProtocol;
EDKII_VARIABLE_POLICY_PROTOCOL *mVariablePolicyProtocol;

STATIC EFI_STATUS InitializeFrontPageUI(VOID)
{
    EFI_STATUS Status = EFI_SUCCESS;

    // Establish initial FrontPage TitleBar and Master Frame dimensions based on the current screen size.
    //
    mTitleBarWidth = mBootHorizontalResolution;
    mTitleBarHeight = ((mBootVerticalResolution * FP_TBAR_HEIGHT_PERCENT) / 100);
    mMasterFrameWidth = ((mBootHorizontalResolution * FP_MFRAME_WIDTH_PERCENT) / 100);
    mMasterFrameHeight = (mBootVerticalResolution - mTitleBarHeight);

    DEBUG((DEBUG_INFO, "INFO [FP]: FP Dimensions: %d, %d, %d, %d, %d, %d\r\n",
           mBootHorizontalResolution, mBootVerticalResolution, mTitleBarWidth, mTitleBarHeight, mMasterFrameWidth, mMasterFrameHeight));

    // Compute Master Frame origin and menu text indentation.
    //
    UINT32 MasterFrameMenuOrigX = 0;
    UINT32 MasterFrameMenuOrigY = mTitleBarHeight;
    UINT32 CellTextXOffset = ((mMasterFrameWidth * FP_MFRAME_MENU_TEXT_OFFSET_PERCENT) / 100);

    // Determine whether there are any events that require user notification.
    // NOTE: This should come before CreateTopMenu() because it needs to happen before the
    //       Admin Password prompt.
    //
    NotifyUserOfAlerts();

    // Create the top-level menu in the Master Frame.
    //
    mTopMenu = CreateTopMenu(MasterFrameMenuOrigX,
                             MasterFrameMenuOrigY,
                             (mMasterFrameWidth - FP_MFRAME_DIVIDER_LINE_WIDTH_PIXELS),
                             ((mMasterFrameHeight * FP_MFRAME_MENU_CELL_HEIGHT_PERCENT) / 100),
                             CellTextXOffset);

    ASSERT(NULL != mTopMenu);
    if (NULL == mTopMenu)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Render the TitleBar at the top of the screen.
    //
    RenderTitlebar();

    // Render the Master Frame and its Top-Level menu contents.
    //
    //RenderMasterFrame();

    // Create the Master Frame notification event.  This event is signalled by the display engine to note that
    // there is a user input event outside the form area to consider.
    //
    Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                                TPL_CALLBACK,
                                MasterFrameNotifyCallback,
                                NULL,
                                &gMsEventMasterFrameNotifyGroupGuid,
                                &mMasterFrameNotifyEvent);

    if (EFI_SUCCESS != Status)
    {
        DEBUG((DEBUG_ERROR, "ERROR [FP]: Failed to create master frame notification event.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Set shared pointer to user input context structure in a PCD so it can be shared.
    //
    PcdSet64S(PcdCurrentPointerState, (UINT64)(UINTN)&mDisplayEngineState);

Exit:

    return Status;
}

EFI_STATUS CallFrontPage(UINT32 FormIndex)
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINT16 Count, Index = 0;
    EFI_BROWSER_ACTION_REQUEST ActionRequest;

#define MAX_FORMSET_HANDLES 5
    EFI_HII_HANDLE Handles[MAX_FORMSET_HANDLES];
    UINTN HandleCount;

    // Locate Boot Menu form - this should already be registered.
    //
    EFI_GUID BootMenu = MS_BOOT_MENU_FORMSET_GUID;
    EFI_HII_HANDLE *BootHandle = HiiGetHiiHandles(&BootMenu);
    EFI_HII_HANDLE *DfciHandle = HiiGetHiiHandles(&gDfciMenuFormsetGuid);
    EFI_HII_HANDLE *HwhHandle = HiiGetHiiHandles(&gHwhMenuFormsetGuid);

    Handles[0] = mFrontPagePrivate.HiiHandle;
    HandleCount = 1;

    if (BootHandle != NULL)
    {
        Handles[HandleCount++] = BootHandle[0];
        FreePool(BootHandle);
    }
    if (DfciHandle != NULL)
    {
        Handles[HandleCount++] = DfciHandle[0];
        FreePool(DfciHandle);
    }
    if (HwhHandle != NULL)
    {
        Handles[HandleCount++] = HwhHandle[0];
        FreePool(HwhHandle);
    }

    DEBUG((DEBUG_INFO, "MAX_FORMSET_HANDLES=%d, CurrentFormsetHandles=%d\n", MAX_FORMSET_HANDLES, HandleCount));
    ASSERT(HandleCount < MAX_FORMSET_HANDLES);

    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

    // Search through the form mapping table to find the form set GUID and ID corresponding to the selected index.
    //
    for (Count = 0; Count < (sizeof(mFormMap) / sizeof(mFormMap[0])); Count++)
    {
        Index = ((FALSE == mShowFullMenu) ? mFormMap[Count].LimitedMenuIndex : mFormMap[Count].FullMenuIndex);

        if (Index == FormIndex)
        {
            break;
        }
    }

    // If we didn't find it, exit with an error.
    //
    if (Index != FormIndex)
    {
        Status = EFI_NOT_FOUND;
        goto Exit;
    }

    // Call the browser to display the selected form.
    //
    Status = mFormBrowser2->SendForm(mFormBrowser2,
                                     Handles,
                                     HandleCount,
                                     &mFormMap[Count].FormSetGUID,
                                     mFormMap[Count].FormId,
                                     (EFI_SCREEN_DESCRIPTOR *)NULL,
                                     &ActionRequest);

    // If the user selected the "Restart now" button to exit the Frontpage, set the exit flag.
    //
    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_EXIT)
    {
        mTerminateFrontPage = TRUE;
    }

    // Check whether user change any option setting which needs a reset to be effective
    //
    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET)
    {
        mResetRequired = TRUE;
    }

Exit:

    return Status;
}

/**
  This function is the main entry of the platform setup entry.
  The function will present the main menu of the system setup,
  this is the platform reference part and can be customize.
**/
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status = EFI_SUCCESS;

    //Delete BootNext if entry to BootManager.
    //Status = gRT->SetVariable(L"BootNext", &gEfiGlobalVariableGuid, 0, 0, NULL);

    // Save image handle for later.
    //
    mImageHandle = ImageHandle;

    // Disable the watchdog timer
    //
    // gBS->SetWatchdogTimer(0, 0, 0, (CHAR16 *)NULL);

    // mResetRequired = FALSE;

    // Status = gBS->LocateProtocol(&gDfciSettingAccessProtocolGuid, NULL, (VOID **)&mSettingAccess);
    // if (EFI_ERROR(Status))
    // {
    //     ASSERT_EFI_ERROR(Status);
    //     DEBUG((DEBUG_ERROR, "%a Couldn't locate system setting access protocol\n", __FUNCTION__));
    // }

    // Force-connect all controllers.
    //
    EfiBootManagerConnectAll();

    // Set console mode: *not* VGA, no splashscreen logo.
    // Insure Gop is in Big Display mode prior to accessing GOP.
    //SetGraphicsConsoleMode(GCM_NATIVE_RES);

    //
    // After the console is ready, get current video resolution
    // and text mode before launching setup at first time.
    //
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);

    if (EFI_ERROR(Status))
    {
        mGop = (EFI_GRAPHICS_OUTPUT_PROTOCOL *)NULL;
        goto Exit;
    }

    // Determine if the Font Protocol is available
    //
    Status = gBS->LocateProtocol(&gEfiHiiFontProtocolGuid, NULL, (VOID **)&mFont);

    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
    {
        mFont = (EFI_HII_FONT_PROTOCOL *)NULL;
        Status = EFI_UNSUPPORTED;
        DEBUG((DEBUG_ERROR, "ERROR [FP]: Failed to find Font protocol (%r).\r\n", Status));
        goto Exit;
    }

    // Locate the Simple Window Manager protocol.
    //
    // Status = gBS->LocateProtocol(&gMsSWMProtocolGuid, NULL, (VOID **)&mSWMProtocol);

    // if (EFI_ERROR(Status))
    // {
    //     mSWMProtocol = NULL;
    //     Status = EFI_UNSUPPORTED;
    //     DEBUG((DEBUG_ERROR, "ERROR [FP]: Failed to find the window manager protocol (%r).\r\n", Status));
    //     goto Exit;
    // }

    if (mGop != NULL)
    {
        //
        // Get current video resolution and text mode.
        //
        mBootHorizontalResolution = mGop->Mode->Info->HorizontalResolution;
        mBootVerticalResolution = mGop->Mode->Info->VerticalResolution;
    }

    // Ensure screen is clear when switch Console from Graphics mode to Text mode
    //
    gST->ConOut->EnableCursor(gST->ConOut, FALSE);
    gST->ConOut->ClearScreen(gST->ConOut);

    // Initialize the Simple UI ToolKit.
    //
    Status = InitializeUIToolKit(ImageHandle);

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [FP]: Failed to initialize the UI toolkit (%r).\r\n", Status));
        goto Exit;
    }

    // Register Front Page strings with the HII database.
    //
    //InitializeStringSupport();

    // Initialize HII data (ex: register strings, etc.).
    //
    InitializeFrontPage(TRUE);

    // Initialize the FrontPage User Interface.
    //
    Status = InitializeFrontPageUI();

    if (EFI_SUCCESS != Status)
    {
        DEBUG((DEBUG_ERROR, "ERROR [FP]: Failed to initialize the FrontPage user interface.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Set the default form ID to show on the canvas.
    //
    mCurrentFormIndex = 0;
    Status = EFI_SUCCESS;

    // Display the specified FrontPage form.
    //
    do
    {
        // By default, we'll terminate FrontPage after processing the next Form unless the flag is reset.
        //
        mTerminateFrontPage = TRUE;

        CallFrontPage(mCurrentFormIndex);

    } while (FALSE == mTerminateFrontPage);

    if (mResetRequired)
    {
        ResetSystemWithSubtype(EfiResetCold, &gFrontPageResetGuid);
    }

    ProcessBootNext();

    // Clean-up
    //
    UninitializeFrontPage();

Exit:

    return Status;
}