/** @file
  Bit field functions of BaseLib.

  @copyright
  INTEL CONFIDENTIAL
  Copyright 2006 - 2013 Intel Corporation. <BR>

  The source code contained or described herein and all documents related to the
  source code ("Material") are owned by Intel Corporation or its suppliers or
  licensors. Title to the Material remains with Intel Corporation or its suppliers
  and licensors. The Material may contain trade secrets and proprietary    and
  confidential information of Intel Corporation and its suppliers and licensors,
  and is protected by worldwide copyright and trade secret laws and treaty
  provisions. No part of the Material may be used, copied, reproduced, modified,
  published, uploaded, posted, transmitted, distributed, or disclosed in any way
  without Intel's prior express written permission.

  No license under any patent, copyright, trade secret or other intellectual
  property right is granted to or conferred upon you by disclosure or delivery
  of the Materials, either expressly, by implication, inducement, estoppel or
  otherwise. Any license under such intellectual property rights must be
  express and approved by Intel in writing.

  Unless otherwise agreed by Intel in writing, you may not remove or alter
  this notice or any other notice embedded in Materials by Intel or
  Intel's suppliers or licensors in any way.
**/
#include <string.h>
#include <dbgout.h>
#include "peciifc.h"
#include "bios.h"


/*---------- LShiftU64.c/RShiftU64.c: START ----------*/
/**
  Shifts a 64-bit integer right between 0 and 63 bits. This high bits are
  filled with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the right by Count bits. The
  high Count bits are set to zero. The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift right.
  @param  Count   The number of bits to shift right.

  @return Operand >> Count.

**/
UINT64
EFIAPI
RShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  ASSERT (Count < 64);
  //return InternalMathRShiftU64 (Operand, Count);
  return Operand >> Count; /* from Math64.c */
}

/**
  Shifts a 64-bit integer left between 0 and 63 bits. The low bits are filled
  with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the left by Count bits. The
  low Count bits are set to zero. The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift left.
  @param  Count   The number of bits to shift left.

  @return Operand << Count.

**/
UINT64
EFIAPI
LShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  ASSERT (Count < 64);
  //return InternalMathLShiftU64 (Operand, Count);
  return Operand << Count; /* from Math64.c */
}
/*---------- END ----------*/

UINT64
EFIAPI
InternalMathDivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  return Dividend / Divisor;
}


/*---------- ModU64x32.c: START ----------*/
UINT32
EFIAPI
ModU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  ASSERT (Divisor != 0);
  //return InternalMathModU64x32 (Dividend, Divisor);
  return (UINT32)(Dividend % Divisor);
}
/*---------- END ----------*/

/**
  Worker function that returns a bit field from Operand.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
  @param  EndBit    The ordinal of the most significant bit in the bit field.

  @return The bit field read.

**/
UINTN
EFIAPI
InternalBaseLibBitFieldReadUint (
  IN      UINTN                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  //
  // ~((UINTN)-2 << EndBit) is a mask in which bit[0] thru bit[EndBit]
  // are 1's while bit[EndBit + 1] thru the most significant bit are 0's.
  //
  return (Operand & ~((UINTN)-2 << EndBit)) >> StartBit;
}

/**
  Worker function that reads a bit field from Operand, performs a bitwise OR,
  and returns the result.

  Performs a bitwise OR between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new value is returned.

  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
  @param  OrData    The value to OR with the read value from the value.

  @return The new value.

**/
UINTN
EFIAPI
InternalBaseLibBitFieldOrUint (
  IN      UINTN                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINTN                     OrData
  )
{
  //
  // Higher bits in OrData those are not used must be zero. 
  //
  // EndBit - StartBit + 1 might be 32 while the result right shifting 32 on a 32bit integer is undefined,
  // So the logic is updated to right shift (EndBit - StartBit) bits and compare the last bit directly.
  //
  ASSERT ((OrData >> (EndBit - StartBit)) == ((OrData >> (EndBit - StartBit)) & 1));
  
  //
  // ~((UINTN)-2 << EndBit) is a mask in which bit[0] thru bit[EndBit]
  // are 1's while bit[EndBit + 1] thru the most significant bit are 0's.
  //
  return Operand | ((OrData << StartBit) & ~((UINTN) -2 << EndBit));
}

/**
  Worker function that reads a bit field from Operand, performs a bitwise AND,
  and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new value is returned.

  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
  @param  AndData    The value to And with the read value from the value.

  @return The new value.

**/
UINTN
EFIAPI
InternalBaseLibBitFieldAndUint (
  IN      UINTN                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINTN                     AndData
  )
{
  //
  // Higher bits in AndData those are not used must be zero. 
  //
  // EndBit - StartBit + 1 might be 32 while the result right shifting 32 on a 32bit integer is undefined,
  // So the logic is updated to right shift (EndBit - StartBit) bits and compare the last bit directly.
  //
  ASSERT ((AndData >> (EndBit - StartBit)) == ((AndData >> (EndBit - StartBit)) & 1));

  //
  // ~((UINTN)-2 << EndBit) is a mask in which bit[0] thru bit[EndBit]
  // are 1's while bit[EndBit + 1] thru the most significant bit are 0's.
  //
  return Operand & ~((~AndData << StartBit) & ~((UINTN)-2 << EndBit));
}

/**
  Returns a bit field from an 8-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.

  @return The bit field read.

**/
UINT8
EFIAPI
BitFieldRead8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT (EndBit < 8);
  ASSERT (StartBit <= EndBit);
  return (UINT8)InternalBaseLibBitFieldReadUint (Operand, StartBit, EndBit);
}

/**
  Writes a bit field to an 8-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 8-bit value is
  returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If Value is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  Value     The new value of the bit field.

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldWrite8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     Value
  )
{
  ASSERT (EndBit < 8);
  ASSERT (StartBit <= EndBit);
  return BitFieldAndThenOr8 (Operand, StartBit, EndBit, 0, Value);
}

/**
  Reads a bit field from an 8-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 8-bit value is returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  OrData    The value to OR with the read value from the value.

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldOr8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     OrData
  )
{
  ASSERT (EndBit < 8);
  ASSERT (StartBit <= EndBit);
  return (UINT8)InternalBaseLibBitFieldOrUint (Operand, StartBit, EndBit, OrData);
}

/**
  Reads a bit field from an 8-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 8-bit value is returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  AndData   The value to AND with the read value from the value.

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldAnd8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData
  )
{
  ASSERT (EndBit < 8);
  ASSERT (StartBit <= EndBit);
  return (UINT8)InternalBaseLibBitFieldAndUint (Operand, StartBit, EndBit, AndData);
}

/**
  Reads a bit field from an 8-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise 
  OR with value specified by OrData. All other bits in Operand are
  preserved. The new 8-bit value is returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldAndThenOr8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData,
  IN      UINT8                     OrData
  )
{
  ASSERT (EndBit < 8);
  ASSERT (StartBit <= EndBit);
  return BitFieldOr8 (
           BitFieldAnd8 (Operand, StartBit, EndBit, AndData),
           StartBit,
           EndBit,
           OrData
           );
}

/**
  Returns a bit field from a 16-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.

  @return The bit field read.

**/
UINT16
EFIAPI
BitFieldRead16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT (EndBit < 16);
  ASSERT (StartBit <= EndBit);
  return (UINT16)InternalBaseLibBitFieldReadUint (Operand, StartBit, EndBit);
}

/**
  Writes a bit field to a 16-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 16-bit value is
  returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If Value is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  Value     The new value of the bit field.

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldWrite16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    Value
  )
{
  ASSERT (EndBit < 16);
  ASSERT (StartBit <= EndBit);
  return BitFieldAndThenOr16 (Operand, StartBit, EndBit, 0, Value);
}

/**
  Reads a bit field from a 16-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 16-bit value is returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  OrData    The value to OR with the read value from the value.

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldOr16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    OrData
  )
{
  ASSERT (EndBit < 16);
  ASSERT (StartBit <= EndBit);
  return (UINT16)InternalBaseLibBitFieldOrUint (Operand, StartBit, EndBit, OrData);
}

/**
  Reads a bit field from a 16-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 16-bit value is returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  AndData   The value to AND with the read value from the value.

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldAnd16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData
  )
{
  ASSERT (EndBit < 16);
  ASSERT (StartBit <= EndBit);
  return (UINT16)InternalBaseLibBitFieldAndUint (Operand, StartBit, EndBit, AndData);
}

/**
  Reads a bit field from a 16-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise 
  OR with value specified by OrData. All other bits in Operand are
  preserved. The new 16-bit value is returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldAndThenOr16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData,
  IN      UINT16                    OrData
  )
{
  ASSERT (EndBit < 16);
  ASSERT (StartBit <= EndBit);
  return BitFieldOr16 (
           BitFieldAnd16 (Operand, StartBit, EndBit, AndData),
           StartBit,
           EndBit,
           OrData
           );
}

/**
  Returns a bit field from a 32-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.

  @return The bit field read.

**/
UINT32
EFIAPI
BitFieldRead32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT (EndBit < 32);
  ASSERT (StartBit <= EndBit);
  return (UINT32)InternalBaseLibBitFieldReadUint (Operand, StartBit, EndBit);
}

/**
  Writes a bit field to a 32-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 32-bit value is
  returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If Value is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  Value     The new value of the bit field.

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldWrite32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    Value
  )
{
  ASSERT (EndBit < 32);
  ASSERT (StartBit <= EndBit);
  return BitFieldAndThenOr32 (Operand, StartBit, EndBit, 0, Value);
}

/**
  Reads a bit field from a 32-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 32-bit value is returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  OrData    The value to OR with the read value from the value.

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldOr32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    OrData
  )
{
  ASSERT (EndBit < 32);
  ASSERT (StartBit <= EndBit);
  return (UINT32)InternalBaseLibBitFieldOrUint (Operand, StartBit, EndBit, OrData);
}

/**
  Reads a bit field from a 32-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 32-bit value is returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the value.

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldAnd32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData
  )
{
  ASSERT (EndBit < 32);
  ASSERT (StartBit <= EndBit);
  return (UINT32)InternalBaseLibBitFieldAndUint (Operand, StartBit, EndBit, AndData);
}

/**
  Reads a bit field from a 32-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise 
  OR with value specified by OrData. All other bits in Operand are
  preserved. The new 32-bit value is returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldAndThenOr32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  )
{
  ASSERT (EndBit < 32);
  ASSERT (StartBit <= EndBit);
  return BitFieldOr32 (
           BitFieldAnd32 (Operand, StartBit, EndBit, AndData),
           StartBit,
           EndBit,
           OrData
           );
}

/**
  Returns a bit field from a 64-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.

  @return The bit field read.

**/
UINT64
EFIAPI
BitFieldRead64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT (EndBit < 64);
  ASSERT (StartBit <= EndBit);
  return RShiftU64 (Operand & ~LShiftU64 ((UINT64)-2, EndBit), StartBit);
}

/**
  Writes a bit field to a 64-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 64-bit value is
  returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If Value is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  Value     The new value of the bit field.

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldWrite64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    Value
  )
{
  ASSERT (EndBit < 64);
  ASSERT (StartBit <= EndBit);
  return BitFieldAndThenOr64 (Operand, StartBit, EndBit, 0, Value);
}

/**
  Reads a bit field from a 64-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 64-bit value is returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  OrData    The value to OR with the read value from the value

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldOr64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    OrData
  )
{
  UINT64  Value1;
  UINT64  Value2;

  ASSERT (EndBit < 64);
  ASSERT (StartBit <= EndBit);
  //
  // Higher bits in OrData those are not used must be zero. 
  //
  // EndBit - StartBit + 1 might be 64 while the result right shifting 64 on RShiftU64() API is invalid,
  // So the logic is updated to right shift (EndBit - StartBit) bits and compare the last bit directly.
  //
  ASSERT (RShiftU64 (OrData, EndBit - StartBit) == (RShiftU64 (OrData, EndBit - StartBit) & 1));

  Value1 = LShiftU64 (OrData, StartBit);
  Value2 = LShiftU64 ((UINT64) - 2, EndBit);

  return Operand | (Value1 & ~Value2);
}

/**
  Reads a bit field from a 64-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 64-bit value is returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the value.

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldAnd64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData
  )
{
  UINT64  Value1;
  UINT64  Value2;
  
  ASSERT (EndBit < 64);
  ASSERT (StartBit <= EndBit);
  //
  // Higher bits in AndData those are not used must be zero. 
  //
  // EndBit - StartBit + 1 might be 64 while the right shifting 64 on RShiftU64() API is invalid,
  // So the logic is updated to right shift (EndBit - StartBit) bits and compare the last bit directly.
  //
  ASSERT (RShiftU64 (AndData, EndBit - StartBit) == (RShiftU64 (AndData, EndBit - StartBit) & 1));

  Value1 = LShiftU64 (~AndData, StartBit);
  Value2 = LShiftU64 ((UINT64)-2, EndBit);

  return Operand & ~(Value1 & ~Value2);
}

/**
  Reads a bit field from a 64-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise 
  OR with value specified by OrData. All other bits in Operand are
  preserved. The new 64-bit value is returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().
  If AndData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().
  If OrData is larger than the bitmask value range specified by StartBit and EndBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldAndThenOr64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  )
{
  ASSERT (EndBit < 64);
  ASSERT (StartBit <= EndBit);
  return BitFieldOr64 (
           BitFieldAnd64 (Operand, StartBit, EndBit, AndData),
           StartBit,
           EndBit,
           OrData
           );
}

/** HighBitSet32/64.c
  Returns the bit position of the highest bit set in a 64-bit value. Equivalent
  to log2(x).

  This function computes the bit position of the highest bit set in the 64-bit
  value specified by Operand. If Operand is zero, then -1 is returned.
  Otherwise, a value between 0 and 63 is returned.

  @param  Operand The 64-bit operand to evaluate.

  @retval 0..63   Position of the highest bit set in Operand if found.
  @retval -1      Operand is zero.

**/
INTN
EFIAPI
HighBitSet32 (
  IN      UINT32                    Operand
  )
{
  INTN                              BitIndex;

  if (Operand == 0) {
    return - 1;
  }
  for (BitIndex = 31; (INT32)Operand > 0; BitIndex--, Operand <<= 1);
  return BitIndex;
}

INTN
EFIAPI
HighBitSet64 (
  IN      UINT64                    Operand
  )
{
  if (Operand == (UINT32)Operand) {
    //
    // Operand is just a 32-bit integer
    //
    return HighBitSet32 ((UINT32)Operand);
  }

  //
  // Operand is really a 64-bit integer
  //
  if (sizeof (UINTN) == sizeof (UINT32)) {
    return HighBitSet32 (((UINT32*)&Operand)[1]) + 32;
  } else {
    return HighBitSet32 ((UINT32)RShiftU64 (Operand, 32)) + 32;
  }
}


/** DivU64x32.c
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and generates
  a 64-bit unsigned result.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. This
  function returns the 64-bit unsigned quotient.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.

  @return Dividend / Divisor

**/
UINT64
EFIAPI
DivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  ASSERT (Divisor != 0);
  return InternalMathDivU64x32 (Dividend, Divisor);
}


/**
  Reads CPU Uncore & IIO PCI configuration space using the MMIO mechanism
  @param[in] SocId    - CPU Socket Node number
  @param[in] BoxInst  - Box Instance, 0 based
  @param[in] Offset   - Register offset; values come from the auto generated header file
  @retval Register value

**/
UINT32
EFIAPI
ReadCpuCsrMmio(
  endPointRegInfo *reg
  )
{
	INT8 	dev_id = 0;
	UINT8   target = reg->dimmBdf.socket + MIN_CPU_ADDRESS;
	INT8 	domain = 0;
	INT8	xmit_feedback = 0;
	INT8	awfcs = 1;
	INT32   readLen = 1;

	peci_rdendpointconfig_mmio_req_t	rdMMIOReq = {0};
	peci_rdendpointconfig_mmio_res_t	rdMMIORes = {0};
	UINT32 resData;
	UINT8 devFunc = (reg->device<<3) | reg->function;

	if(MESSAGE_MMIO != reg->msgType){
		TCRIT("Wrong message type: %d\n", reg->msgType);
		return 0xffffffff;
	}

	resData = 0;

	rdMMIOReq.option = reg->optType;
	rdMMIOReq.host_id = 0;
	rdMMIOReq.bar = 0; /*reg->bar_id;*/
	rdMMIOReq.bus = reg->dimmBdf.bus;
	rdMMIOReq.devicefun = devFunc;
	rdMMIOReq.segment = 0;
	rdMMIOReq.message_type = reg->msgType; /* peci-6.15 and libpeci-6.17 need this setting */
	rdMMIOReq.address_type = reg->addrType;
	rdMMIOReq.pci_config_addr = reg->offset;
	if(reg->dimmBdf.channel % NUMBER_OF_CHANNELS) {
		//TINFO("Odd MMIO read: ch %d, offset 0x%08lx\n", reg->dimmBdf.channel,reg->offset);
		rdMMIOReq.pci_config_addr |= MMIO_CHANNEL_OFFSET_DIFF;
	}

	if ( 0 != peci_cmd_rdendpointconfig_mmio(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdMMIOReq, &rdMMIORes)) {
		TCRIT("peci_cmd_rdendpointconfig_mmio failed, CC=0x%x\n", rdMMIORes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",rdMMIOReq.option);
		TDBG("\tbus:             %d",rdMMIOReq.bus);
		TDBG("\tdevicefun:       %d",rdMMIOReq.devicefun);
		TDBG("\taddress_type:    %d",rdMMIOReq.address_type);
		TDBG("\tchannel:         %d",reg->dimmBdf.channel);
		TDBG("\tpci_config_addr: 0x%llx\n",rdMMIOReq.pci_config_addr);
		return 0xffffffff;
	}

	memcpy(&resData, &rdMMIORes.data, rdMMIOReq.option);
	return resData;
}

/**
  Writes CPU Uncore & IIO PCI configuration space using the MMIO mechanism
  @param[in] SocId     - CPU Socket Node number
  @param[in] BoxInst   - Box Instance, 0 based
  @param[in] Offset    - Register offset; values come from the auto generated header file
  @param[in] Data      - Register data to be written
  @retval None
**/
int
EFIAPI
WriteCpuCsrMmio(
  endPointRegInfo *reg,
  UINT32   Data
  )
{
	INT8 	dev_id = 0;
	UINT8   target = reg->dimmBdf.socket + MIN_CPU_ADDRESS;
	INT8 	domain = 0;
	INT8	xmit_feedback = 0;
	INT8	awfcs = 1;
	INT32   readLen = 1;

	peci_wrendpointconfig_mmio_req_t	wrMMIOReq = {0};
	peci_wrendpointconfig_mmio_res_t	wrMMIORes = {0};
	UINT8 devFunc = (reg->device<<3) | reg->function;

	if(MESSAGE_MMIO != reg->msgType){
		TCRIT("Wrong message type: %d\n", reg->msgType);
		return -1;
	}

	wrMMIOReq.option = reg->optType;
	wrMMIOReq.host_id = 0;
	wrMMIOReq.bar = 0; /*reg->bar_id;*/
	wrMMIOReq.bus = reg->dimmBdf.bus;
	wrMMIOReq.devicefun = devFunc;
	wrMMIOReq.segment = 0;
	wrMMIOReq.message_type = reg->msgType; /* peci-6.15 and libpeci-6.17 need this setting */
	wrMMIOReq.address_type = reg->addrType;
	wrMMIOReq.pci_config_addr = reg->offset;
	wrMMIOReq.data.peci_dword = Data;
	if(reg->dimmBdf.channel % NUMBER_OF_CHANNELS) {
		//TINFO("Odd MMIO write: ch %d, offset 0x%08lx\n", reg->dimmBdf.channel,reg->offset);
		wrMMIOReq.pci_config_addr |= MMIO_CHANNEL_OFFSET_DIFF;
	}

	if ( 0 != peci_cmd_wrendpointconfig_mmio(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &wrMMIOReq, &wrMMIORes)) {
		TCRIT("peci_cmd_wrendpointconfig_mmio failed, CC=0x%x", wrMMIORes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",wrMMIOReq.option);
		TDBG("\tbus:             %d",wrMMIOReq.bus);
		TDBG("\tdevicefun:       %d",wrMMIOReq.devicefun);
		TDBG("\taddress_type:    %d",wrMMIOReq.address_type);
		TDBG("\tchannel:         %d",reg->dimmBdf.channel);
		TDBG("\tpci_config_addr: 0x%llx\n",wrMMIOReq.pci_config_addr);
		return -1;
	}
	return 0;

}

UINT32 EFIAPI ReadCpuCsrPciLocal(
  endPointRegInfo *reg
  )
{
	INT8 	dev_id = 0;
	UINT8   target = reg->dimmBdf.socket + MIN_CPU_ADDRESS;
	INT8 	domain = 0;
	INT8	xmit_feedback = 0;
	INT8	awfcs = 1;
	INT32   readLen = 1;

	peci_rdendpointconfig_pci_req_t	rdEndPntPciReq = {0};
	peci_rdendpointconfig_pci_res_t	reEndPntPciRes = {0};
	UINT32 data;

	if(MESSAGE_LOCAL_PCI != reg->msgType){
		TCRIT("Wrong message type: %d\n", reg->msgType);
		return 0xffffffff;
	}

	rdEndPntPciReq.option = reg->optType;
	rdEndPntPciReq.host_id = 0;
	rdEndPntPciReq.message_type = reg->msgType;
	rdEndPntPciReq.segment = 0;
	rdEndPntPciReq.pci_config_addr = PCI_CFG_ADDR(reg->dimmBdf.bus, reg->device, reg->function, reg->offset);

	if ( 0 != peci_cmd_rdendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdEndPntPciReq, &reEndPntPciRes) ) {
		TCRIT("peci_cmd_rdendpointconfig_pci failed, CC=0x%x", reEndPntPciRes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",rdEndPntPciReq.option);
		TDBG("\tmessage_type:    %d",rdEndPntPciReq.message_type);
		TDBG("\tsocket:          %d",reg->dimmBdf.socket);
		TDBG("\tBus:             %d",reg->dimmBdf.bus);
		TDBG("\tDevice:          %d",reg->device);
		TDBG("\tFunction:        %d",reg->function);
		TDBG("\tOffset:          0x%llx\n",reg->offset);
	if ( reEndPntPciRes.completion_code != 0x40 ) {
#ifdef PECI_OPTION_TEST
		/* check if DWORD works instead of QWORD */
		if(OPTION_QWORD == rdEndPntPciReq.option) {
			rdEndPntPciReq.option = OPTION_DWORD;
				if ( 0 != peci_cmd_rdendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &rdEndPntPciReq, &reEndPntPciRes) ) {
				TCRIT("peci_cmd_rdendpointconfig_pci failed, CC=0x%x\n", reEndPntPciRes.completion_code);
				return 0xffffffff;
			}
			if ( reEndPntPciRes.completion_code == 0x40 ) {
				TCRIT("peci_cmd_rdendpointconfig_pci succeeded with DWORD option to access pci_config_addr =0x%x\n", rdEndPntPciReq.pci_config_addr);
			}
		}
#endif
		return 0xffffffff;
	}

		return 0xffffffff;
	}
	memcpy(&data, &reEndPntPciRes.data, rdEndPntPciReq.option);
	return data;
}

int EFIAPI WriteCpuCsrPciLocal(
  endPointRegInfo *reg,
  UINT32 Data
  )
{
	INT8 	dev_id = 0;
	UINT8   target = reg->dimmBdf.socket + MIN_CPU_ADDRESS;
	INT8 	domain = 0;
	INT8	xmit_feedback = 0;
	INT8	awfcs = 1;
	INT32   readLen = 1;

	peci_wrendpointconfig_pci_req_t wrEndPntPciReq = {0};
	peci_wrendpointconfig_pci_res_t wrEndPntPciRes = {0};

	if(MESSAGE_LOCAL_PCI != reg->msgType){
		TCRIT("Wrong message type: %d\n", reg->msgType);
		return -1;
	}

	wrEndPntPciReq.option = reg->optType;
	wrEndPntPciReq.host_id = 0;
	wrEndPntPciReq.message_type = reg->msgType;
	wrEndPntPciReq.segment = 0;
	wrEndPntPciReq.pci_config_addr = PCI_CFG_ADDR(reg->dimmBdf.bus, reg->device, reg->function, reg->offset);
	wrEndPntPciReq.data.peci_dword = Data;

	if ( 0 != peci_cmd_wrendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &wrEndPntPciReq, &wrEndPntPciRes)) {
		TCRIT("peci_cmd_wrendpointconfig_pci failed, CC=0x%x", wrEndPntPciRes.completion_code);
		TDBG("\ttarget:          %d",target);
		TDBG("\toption:          %d",wrEndPntPciReq.option);
		TDBG("\tmessage_type:    %d",wrEndPntPciReq.message_type);
		TDBG("\tsocket:          %d",reg->dimmBdf.socket);
		TDBG("\tBus:             %d",reg->dimmBdf.bus);
		TDBG("\tDevice:          %d",reg->device);
		TDBG("\tFunction:        %d",reg->function);
		TDBG("\tOffset:          0x%llx",reg->offset);
		TDBG("\tdata:            0x%x\n",Data);
	if ( wrEndPntPciRes.completion_code != 0x40 ) {
#ifdef PECI_OPTION_TEST
		/* check if DWORD works instead of QWORD */
		if(OPTION_QWORD == wrEndPntPciReq.option) {
			wrEndPntPciReq.option = OPTION_DWORD;
				if ( 0 != peci_cmd_wrendpointconfig_pci(dev_id, target, domain, xmit_feedback, awfcs, &readLen, &wrEndPntPciReq, &wrEndPntPciRes)) {
				TCRIT("peci_cmd_wrendpointconfig_pci failed, CC=0x%x\n", wrEndPntPciRes.completion_code);
				return -1;
			}
			if ( wrEndPntPciRes.completion_code == 0x40 ) {
				TCRIT("peci_cmd_wrendpointconfig_pci succeeded with DWORD option to access pci_config_addr =0x%x\n", wrEndPntPciReq.pci_config_addr);
			}
		}
#endif
			return -1;
		}

		return -1;
	}
	return 0;
}

