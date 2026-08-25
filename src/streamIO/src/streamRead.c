/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file streamRead.c
 * @brief Implementing the interface to a low-level EXI stream reader
 *
 * @date Aug 18, 2010
 * @author Rumen Kyusakov
 */

#include "streamRead.h"
#include "ioUtil.h"

errorCode readNextBit(EXIStream* strm, bool* bit_val)
{
	if(strm->buffer.bufContent <= strm->context.bufferIndx) // the whole buffer is parsed! read another portion
	{
		strm->context.bitPointer = 0;
		strm->context.bufferIndx = 0;
		strm->buffer.bufContent = 0;
		if(strm->buffer.ioStrm.readWriteToStream == NULL)
			return EXIP_BUFFER_END_REACHED;
		strm->buffer.bufContent = strm->buffer.ioStrm.readWriteToStream(strm->buffer.buf, strm->buffer.bufLen, strm->buffer.ioStrm.stream);
		if(strm->buffer.bufContent == 0)
			return EXIP_BUFFER_END_REACHED;
	}

	*bit_val = (strm->buffer.buf[strm->context.bufferIndx] & (1<<REVERSE_BIT_POSITION(strm->context.bitPointer))) != 0;

	moveBitPointer(strm, 1);
	DEBUG_MSG(INFO, DEBUG_STREAM_IO, ("  @%u:%u", (unsigned int) strm->context.bufferIndx, strm->context.bitPointer));
	return EXIP_OK;
}

errorCode readBits(EXIStream* strm, unsigned char n, unsigned long* bits_val)
{
	unsigned int numBytesToBeRead = 1 + ((n + strm->context.bitPointer - 1) >> 3);
	unsigned int byteIndx = 1;
	unsigned char *buf;

	if(strm->buffer.bufContent < strm->context.bufferIndx + numBytesToBeRead)
	{
		// The buffer end is reached: there are fewer than n bits left unparsed
		errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

		TRY(readEXIChunkForParsing(strm, numBytesToBeRead));
	}

	buf = (unsigned char *) strm->buffer.buf + strm->context.bufferIndx;

	// Left and right shifts clear out the used bits.
	// Replaced (* 8) with (<< 3) to avoid slow multiplication.
	// Cast to unsigned long prevents sign-extension if bits shift into the sign slot.
	unsigned int shiftAmount = (numBytesToBeRead << 3) - 8;
	*bits_val = (unsigned long)(((buf[0] << strm->context.bitPointer) & 0xFF) >> strm->context.bitPointer);
	*bits_val = *bits_val << shiftAmount;

	// Deduct 8 from the shiftAmount register on each loop instead of recalculating multiplication.
	while(byteIndx < numBytesToBeRead)
	{
		shiftAmount -= 8;
		*bits_val += (unsigned long)(buf[byteIndx]) << shiftAmount;
		byteIndx++;
	}

	*bits_val = *bits_val >> ((numBytesToBeRead << 3) - n - strm->context.bitPointer);

	DEBUG_MSG(INFO, DEBUG_STREAM_IO, (">> %lu [0x%lX] (%u bits)", *bits_val, *bits_val, n));

	n += strm->context.bitPointer;
	strm->context.bufferIndx += n >> 3;
	strm->context.bitPointer = n & 7;

	DEBUG_MSG(INFO, DEBUG_STREAM_IO, ("  @%u:%u\n", (unsigned int) strm->context.bufferIndx, strm->context.bitPointer));

	return EXIP_OK;
}

