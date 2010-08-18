/*==================================================================================*\
|                                                                                    |
|                    EXIP - Efficient XML Interchange Processor                      |
|                                                                                    |
|------------------------------------------------------------------------------------|
| Copyright (c) 2010, EISLAB - Luleå University of Technology                        |
| All rights reserved.                                                               |
|                                                                                    |
| Redistribution and use in source and binary forms, with or without                 |
| modification, are permitted provided that the following conditions are met:        |
|     * Redistributions of source code must retain the above copyright               |
|       notice, this list of conditions and the following disclaimer.                |
|     * Redistributions in binary form must reproduce the above copyright            |
|       notice, this list of conditions and the following disclaimer in the          |
|       documentation and/or other materials provided with the distribution.         |
|     * Neither the name of the EISLAB - Luleå University of Technology nor the      |
|       names of its contributors may be used to endorse or promote products         |
|       derived from this software without specific prior written permission.        |
|                                                                                    |
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND    |
| ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED      |
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE             |
| DISCLAIMED. IN NO EVENT SHALL EISLAB - LULEÅ UNIVERSITY OF TECHNOLOGY BE LIABLE    |
| FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES |
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;       |
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND        |
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT         |
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS      |
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                       |
|                                                                                    |
|                                                                                    |
|                                                                                    |
\===================================================================================*/

/**
 * @file streamDecode.c
 * @brief Implementing the interface to a higher-level EXI stream decoder - decode basic EXI types
 *
 * @date Aug 18, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#include "../include/streamDecode.h"

errorCode decodeNBitUnsignedInteger(EXIStream* strm, unsigned char n, unsigned int* int_val)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode decodeBoolean(EXIStream* strm, unsigned char* bool_val)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode decodeUnsignedInteger(EXIStream* strm, unsigned int* int_val)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode decodeString(EXIStream* strm, char* string_val)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode decodeBinary(EXIStream* strm, char* binary_val)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode decodeIntegerValue(EXIStream* strm, int* sint_val)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode decodeDecimalValue(EXIStream* strm, float* dec_val)
{
	return NOT_IMPLEMENTED_YET;
}

errorCode decodeFloatValue()
{
	return NOT_IMPLEMENTED_YET;
}
