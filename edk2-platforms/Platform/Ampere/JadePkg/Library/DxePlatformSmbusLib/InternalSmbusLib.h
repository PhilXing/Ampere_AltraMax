/** @file
  Internal header file for Smbus library.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_SMBUS_LIB_H_
#define INTERNAL_SMBUS_LIB_H_


#include <PiDxe.h>

#include <IndustryStandard/SmBus.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmbusLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SmbusHc.h>

//
// Declaration for internal functions
//
/**
  Executes an SMBus operation to an SMBus controller.

  This function provides a standard way to execute Smbus script
  as defined in the SmBus Specification. The data can either be of
  the Length byte, word, or a block of data.

  @param  SmbusOperation  Signifies which particular SMBus hardware protocol instance
                          that it will use toexecute the SMBus transactions.
  @param  SmBusAddress    The address that encodes the SMBUS Slave Address,
                          SMBUS Command, SMBUS Data Length, and PEC.
  @param  Length          Signifies the number of bytes that this operation will
                          do. The maximum number of bytes can be revision specific
                          and operation specific.
  @param  Buffer          Contains the value of data to execute to the SMBus slave
                          device. Not all operations require this argument. The
                          length of this buffer is identified by Length.
  @param  Status          Return status for the executed command.
                          This is an optional parameter and may be NULL.

  @return The actual number of bytes that are executed for this operation.

**/
UINTN
InternalSmBusExec (
  IN    EFI_SMBUS_OPERATION SmbusOperation,
  IN    UINTN               SmBusAddress,
  IN    UINTN               Length,
  IN OUT VOID               *Buffer,
  OUT    RETURN_STATUS      *Status OPTIONAL
  );

/**
  The constructor function caches the pointer to Smbus protocol.

  The constructor function locates Smbus protocol from protocol database.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmbusLibConstructor (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  );

#endif
