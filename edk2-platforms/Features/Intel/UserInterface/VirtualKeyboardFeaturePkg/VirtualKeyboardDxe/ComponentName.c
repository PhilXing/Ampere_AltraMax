/** @file
  Virtual Keyboard driver name.

  Copyright (c) 2012 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VirtualKeyboard.h"

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL gVirtualKeyboardComponentName = {
  VirtualKeyboardComponentNameGetDriverName,
  VirtualKeyboardComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL gVirtualKeyboardComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     VirtualKeyboardComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) VirtualKeyboardComponentNameGetControllerName,
  "en"
};

//
// Table of driver names
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mVirtualKeyboardDriverNameTable[] = {
  {
    "eng;en",
    L"UEFI Virtual Keyboard Driver"
  },
  {
    NULL,
    NULL
  }
};

//
// Controller name string table
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mVirtualKeyboardControllerNameStringTable[] = {
  {
    "eng",
    L"UEFI Virtual Keyboard Driver"
  },
  {
    NULL,
    NULL
  }
};

/**
  Retrieves a Unicode string that is the user-readable name of the EFI Driver.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param[in]  Language          A pointer to a three-character ISO 639-2 language identifier.
                                This is the language of the driver name that that the caller
                                is requesting, and it must match one of the languages specified
                                in SupportedLanguages.  The number of languages supported by a
                                driver is up to the driver writer.
  @param[out] DriverName        A pointer to the Unicode string to return.  This Unicode string
                                is the name of the driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by This
                                and the language specified by Language was returned
                                in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mVirtualKeyboardDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gVirtualKeyboardComponentName)
           );
}

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an EFI Driver.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
  @param[in]  ControllerHandle  The handle of a controller that the driver specified by
                                This is managing.  This handle specifies the controller
                                whose name is to be returned.
  @param[in]  ChildHandle       The handle of the child controller to retrieve the name
                                of.  This is an optional parameter that may be NULL.  It
                                will be NULL for device drivers.  It will also be NULL
                                for a bus drivers that wish to retrieve the name of the
                                bus controller.  It will not be NULL for a bus driver
                                that wishes to retrieve the name of a child controller.
  @param[in]  Language          A pointer to a three character ISO 639-2 language
                                identifier.  This is the language of the controller name
                                that the caller is requesting, and it must match one
                                of the languages specified in SupportedLanguages.  The
                                number of languages supported by a driver is up to the
                                driver writer.
  @param[out] ControllerName    A pointer to the Unicode string to return.  This Unicode
                                string is the name of the controller specified by
                                ControllerHandle and ChildHandle in the language specified
                                by Language, from the point of view of the driver specified
                                by This.

  @retval EFI_SUCCESS           The Unicode string for the user-readable name in the
                                language specified by Language for the driver
                                specified by This was returned in DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently managing
                                the controller specified by ControllerHandle and
                                ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support the
                                language specified by Language.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  ControllerHandle,
  IN  EFI_HANDLE                  ChildHandle, OPTIONAL
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  //
  // ChildHandle must be NULL for a Device Driver
  //
  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mVirtualKeyboardControllerNameStringTable,
           ControllerName,
           (BOOLEAN)(This == &gVirtualKeyboardComponentName)
           );
}
