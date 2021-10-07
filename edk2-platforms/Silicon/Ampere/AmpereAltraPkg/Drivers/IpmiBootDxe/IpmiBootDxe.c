/** @file
  Process IPMI bootdev command and force the UEFI to boot with selected option.

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/GlobalVariable.h>
#include <IndustryStandard/Ipmi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IpmiCommandLibExt.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePathToText.h>

#define BBS_TYPE_OS_HARDDRIVE             0xFD
#define BBS_TYPE_MENU                     0xFE
#define EFI_LAST_BOOT_ORDER_VARIABLE_NAME L"LastBootOrder"

/**
  Get BBS Type from a messaging device path.

  @param DevicePath[In]       Current node of the Messaging device path.

  @retval UINT16              Return BBS Type.

**/
UINT16
GetBBSTypeFromMessagingDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *Node
  )
{
  UINT16                   Result;
  VENDOR_DEVICE_PATH       *VendorDevicePathNode;

  ASSERT (Node != NULL);

  switch (DevicePathSubType (Node)) {
  case MSG_MAC_ADDR_DP:
    Result = BBS_TYPE_EMBEDDED_NETWORK;
    break;

  case MSG_USB_DP:
    //
    // TODO: Add support for USB devices
    //
    break;

  case MSG_SATA_DP:
  case MSG_NVME_NAMESPACE_DP:
    Result = BBS_TYPE_HARDDRIVE;
    break;

  case MSG_VENDOR_DP:
    VendorDevicePathNode = (VENDOR_DEVICE_PATH *)Node;
    if (&VendorDevicePathNode->Guid != NULL) {
      if (CompareGuid (&VendorDevicePathNode->Guid, &((EFI_GUID)DEVICE_PATH_MESSAGING_SAS))) {
        Result = BBS_TYPE_HARDDRIVE;
      }
    }
    break;

  default:
    Result = BBS_TYPE_UNKNOWN;
    break;
  }

  return Result;
}

/**
  Get BBS Type from a Media device path.

  @param Node[In]          Current node of the Media device path.

  @retval UINT16           Return BBS Type.

**/
UINT16
GetBBSTypeFromMediaDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL *Node
  )
{
  UINT16                             Result;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvDevicePathNode;

  ASSERT (Node != NULL);

  switch (DevicePathSubType (Node)) {
  case MEDIA_CDROM_DP:
    Result = BBS_TYPE_CDROM;
    break;

  case MEDIA_PIWG_FW_FILE_DP:
    FvDevicePathNode = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)Node;
    if (&FvDevicePathNode->FvFileName != NULL) {
      if (CompareGuid (&FvDevicePathNode->FvFileName, PcdGetPtr (PcdBootManagerMenuFile))) {
        Result = BBS_TYPE_MENU;
      }
    }
    break;

  case MEDIA_HARDDRIVE_DP:
    Result = BBS_TYPE_OS_HARDDRIVE;
    break;

  default:
    Result = BBS_TYPE_UNKNOWN;
    break;
  }

  return Result;
}

/**
  Get BBS type of a boot option from its option number.

  @param  OptionNumber[In]        Option number of the Boot option.

  @retval UINT8                   Return BBS type.

**/
UINT8
GetBBSType (
  UINT16 OptionNumber
  )
{
  CHAR16                       OptionName[sizeof ("Boot####")];
  EFI_BOOT_MANAGER_LOAD_OPTION Option;
  EFI_DEVICE_PATH_PROTOCOL     *Node;
  EFI_STATUS                   Status;
  UINT16                       Result;

  UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", OptionNumber);
  Status = EfiBootManagerVariableToLoadOption (OptionName, &Option);
  if (EFI_ERROR (Status)) {
    return BBS_TYPE_UNKNOWN;
  }

  DEBUG_CODE (
    CHAR16 *Str = ConvertDevicePathToText (Option.FilePath, TRUE, TRUE);
    if (Str != NULL) {
      DEBUG ((DEBUG_INFO, "Boot0x%04x: %s\n", OptionNumber, Str));
      FreePool (Str);
    }
    );

  Node = Option.FilePath;
  while (!IsDevicePathEnd (Node)) {
    switch (DevicePathType (Node)) {
    case MEDIA_DEVICE_PATH:
      Result = GetBBSTypeFromMediaDevicePath (Node);
      break;

    case MESSAGING_DEVICE_PATH:
      Result = GetBBSTypeFromMessagingDevicePath (Node);
      break;

    default:
      Result = BBS_TYPE_UNKNOWN;
      break;
    }

    if (Result != BBS_TYPE_UNKNOWN) {
      break;
    }

    Node = NextDevicePathNode (Node);
  }

  return Result;
}

/**
  Build a new BootOrder base on an existed one with the head includes all options of the same selected type.

  @param  BootType[In]            BBS Type to find for
  @param  BootOrder[In]           Pointer to the array of an existed BootOrder.
  @param  BootOrderSize[In]       Size of the existed BootOrder.

  @retval UINT16 *                Pointer to the new BootOrder array.
  @retval NULL                    Can't build the new BootOrder or current BootOrder is good enough.

**/
UINT16 *
BuildBootOrder (
  IN UINT8  BootType,
  IN UINT16 *BootOrder,
  IN UINTN  BootOrderSize
  )
{
  UINT16                       *NewBootOrder;
  UINT16                       *SelectOptions;
  UINT16                       *RemainOptions;
  UINT8                        Temp;
  UINTN                        Index;
  UINTN                        NewCnt;
  UINTN                        RemainCnt;
  UINTN                        SelectCnt;

  ASSERT (BootOrder != NULL);

  NewBootOrder = AllocatePool (BootOrderSize);
  SelectOptions = AllocatePool (BootOrderSize);
  RemainOptions = AllocatePool (BootOrderSize);
  if ((NewBootOrder == NULL)
      || (SelectOptions == NULL)
      || (RemainOptions == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Out of resources", __FUNCTION__));
    goto Exit;
  }

  NewCnt = 0;
  SelectCnt = 0;
  RemainCnt = 0;

  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    Temp = GetBBSType (BootOrder[Index]);
    //
    // Hard drive with OS is a special type of hard drive boot option.
    // Treat it with higher priority than normal hard drive.
    //
    if ((BootType == BBS_TYPE_HARDDRIVE) && (Temp == BBS_TYPE_OS_HARDDRIVE)) {
      NewBootOrder[NewCnt++] = BootOrder[Index];
    } else if (Temp == BootType) {
      SelectOptions[SelectCnt++] = BootOrder[Index];
    } else {
      RemainOptions[RemainCnt++] = BootOrder[Index];
    }
  }

  CopyMem (&SelectOptions[SelectCnt], RemainOptions, RemainCnt * sizeof (UINT16));
  CopyMem (&NewBootOrder[NewCnt], SelectOptions, (SelectCnt + RemainCnt) * sizeof (UINT16));

  if (CompareMem (NewBootOrder, BootOrder, BootOrderSize) != 0) {
    FreePool (SelectOptions);
    FreePool (RemainOptions);
    return NewBootOrder;
  }

Exit:
  if (SelectOptions != NULL) {
    FreePool (SelectOptions);
  }

  if (RemainOptions != NULL) {
    FreePool (RemainOptions);
  }

  if (NewBootOrder != NULL) {
    FreePool (NewBootOrder);
  }

  return NULL;
}

/**
  Convert IPMI device selector to BBS Type.

  @param  DeviceSelector[In]      Input device selector.

  @retval UINT8                   Return BBS Type.

**/
UINT8
DeviceSelectorToBBSType (
  UINT8 DeviceSelector
  )
{
  switch (DeviceSelector) {
  case IPMI_BOOT_DEVICE_SELECTOR_PXE:
    return BBS_TYPE_EMBEDDED_NETWORK;

  case IPMI_BOOT_DEVICE_SELECTOR_HARDDRIVE:
    return BBS_TYPE_HARDDRIVE;

  case IPMI_BOOT_DEVICE_SELECTOR_CD_DVD:
    return BBS_TYPE_CDROM;

  case IPMI_BOOT_DEVICE_SELECTOR_BIOS_SETUP:
    return BBS_TYPE_MENU;

  default:
    return BBS_TYPE_UNKNOWN;
  }
}

/**
  Main function to process the IPMI set bootdev command and force the system to boot with selected boot option.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
VOID
EFIAPI
HandleBootTypeCallback (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS                              Status;
  IPMI_BOOT_FLAGS_INFO                    BootFlags;
  UINT16                                  *CurrBootOrder;
  UINT16                                  *NewBootOrder;
  UINT16                                  FirstBootOption;
  UINT64                                  OsIndication;
  UINT8                                   BootInfoAck;
  UINT8                                   BootType;
  UINTN                                   CurrBootOrderSize;
  UINTN                                   DataSize;

  NewBootOrder = NULL;

  GetEfiGlobalVariable2 (EFI_BOOT_ORDER_VARIABLE_NAME, (VOID **)&CurrBootOrder, &CurrBootOrderSize);
  if (CurrBootOrder == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: BootOrder not found\n", __FUNCTION__));
    return;
  }
  //
  // FIXME: At this point, BootOrder must contain at least one boot option.
  //
  FirstBootOption = CurrBootOrder[0];

  Status = IpmiGetBootInfoAck (&BootInfoAck);
  if (EFI_ERROR (Status) || (BootInfoAck != BOOT_OPTION_HANDLED_BY_BIOS)) {
    goto Exit;
  }

  Status = IpmiGetBootFlags (&BootFlags);
  if (EFI_ERROR (Status) || !BootFlags.IsBootFlagsValid) {
    goto Exit;
  }

  BootType = DeviceSelectorToBBSType (BootFlags.DeviceSelector);
  if (BootType == BBS_TYPE_UNKNOWN) {
    goto Exit;
  }

  NewBootOrder = BuildBootOrder (BootType, CurrBootOrder, CurrBootOrderSize);

  if (NewBootOrder != NULL) {
    //
    // Backup current BootOrder if this is not a persistent request.
    //
    if (!BootFlags.IsPersistent) {
      Status = gRT->SetVariable (
                      EFI_LAST_BOOT_ORDER_VARIABLE_NAME,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
                      | EFI_VARIABLE_NON_VOLATILE,
                      CurrBootOrderSize,
                      CurrBootOrder
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to backup the BootOrder\n", __FUNCTION__));
        goto Exit;
      }
    }

    //
    // Update BootOrder with new value.
    //
    Status = gRT->SetVariable (
                    EFI_BOOT_ORDER_VARIABLE_NAME,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
                    | EFI_VARIABLE_NON_VOLATILE,
                    CurrBootOrderSize,
                    NewBootOrder
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Set BootOrder Variable %r\n", __FUNCTION__, Status));
      goto Exit;
    }

    FirstBootOption = NewBootOrder[0];
  }

  //
  // Update BMC status
  //
  Status = IpmiSetBootInfoAck ();
  if (!EFI_ERROR (Status)) {
    IpmiClearBootFlags ();
  }

Exit:
  //
  // UiApp was registered with HIDDEN attribute and will be ignored by BDS.
  // Therefore, if first BootOption is UiApp, Manually set Boot OS Indication.
  //
  if (GetBBSType (FirstBootOption) == BBS_TYPE_MENU) {
    //
    // On first boot with NVRAM clear, UiApp will be set as first boot option by BDS.
    // If UiApp is not explicitly set by IPMI command in this case, refuse to force UiApp boot.
    //
    if (!(PcdGetBool (PcdNvramErased) && (BootType != BBS_TYPE_MENU))) {
      DataSize = sizeof (UINT64);
      Status = gRT->GetVariable (
                      EFI_OS_INDICATIONS_VARIABLE_NAME,
                      &gEfiGlobalVariableGuid,
                      NULL,
                      &DataSize,
                      &OsIndication
                      );
      OsIndication |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
      Status = gRT->SetVariable (
                      EFI_OS_INDICATIONS_VARIABLE_NAME,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
                      | EFI_VARIABLE_NON_VOLATILE,
                      sizeof (UINT64),
                      &OsIndication
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (NewBootOrder != NULL) {
    FreePool (NewBootOrder);
  }

  FreePool (CurrBootOrder);

  gBS->CloseEvent (Event);
}

/**
  Looking for the backup of previous BootOrder, then restore it with respect to current BootOrder.

  @retval EFI_SUCCESS       The function executed successfully.
  @retval Other             Some error occurs when saving the variables.

**/
EFI_STATUS
RestoreBootOrder (
  VOID
  )
{
  EFI_STATUS Status;
  UINT16     *CurrBootOrder;
  UINT16     *LastBootOrder;
  UINTN      CurrBootOrderSize;
  UINTN      LastBootOrderSize;

  GetEfiGlobalVariable2 (EFI_LAST_BOOT_ORDER_VARIABLE_NAME, (VOID **)&LastBootOrder, &LastBootOrderSize);
  if (LastBootOrder == NULL) {
    return EFI_SUCCESS;
  }

  //
  // On first boot, CentOS Shim might add a new boot option to the head of the BootOrder.
  // If this is true, append that option to the tail of the LastBootOrder.
  //
  GetEfiGlobalVariable2 (EFI_BOOT_ORDER_VARIABLE_NAME, (VOID **)&CurrBootOrder, &CurrBootOrderSize);
  if (LastBootOrderSize == (CurrBootOrderSize - sizeof (UINT16))) {
    CurrBootOrder[CurrBootOrderSize / sizeof (UINT16) - 1] = CurrBootOrder[0];
    CopyMem (CurrBootOrder, LastBootOrder, LastBootOrderSize);
    FreePool (LastBootOrder);
    LastBootOrder = CurrBootOrder;
    LastBootOrderSize = CurrBootOrderSize;
  } else {
    FreePool (CurrBootOrder);
  }

  Status = gRT->SetVariable (
                  EFI_BOOT_ORDER_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS
                  | EFI_VARIABLE_NON_VOLATILE,
                  LastBootOrderSize,
                  LastBootOrder
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to restore the BootOrder\n"));
    goto Exit;
  }

  Status = gRT->SetVariable (
                  EFI_LAST_BOOT_ORDER_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  0,
                  0,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to erase the LastBootOrder\n"));
    goto Exit;
  }

Exit:
  FreePool (LastBootOrder);

  return Status;
}

/**
  The Entry Point for IPMI Boot handler.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
IpmiBootEntry (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_EVENT  EndOfDxeEvent;

  RestoreBootOrder ();

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  HandleBootTypeCallback,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
