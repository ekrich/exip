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
 * @file memManagement.c
 * @brief Implementation of handling memory operations - allocation, deallocation etc.
 * @date Oct 25, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#include "memManagement.h"

void* memManagedAllocate(struct memAlloc** mStack, size_t size)
{
	void* ptr = EXIP_MALLOC(size);
	if(ptr != NULL)
	{
		struct memAlloc* memNode =  EXIP_MALLOC(sizeof(struct memAlloc));
		if(memNode != NULL)
		{
			memNode->allocation = ptr;
			memNode->nextAlloc = *mStack;
			*mStack = memNode;
		}
		else
			return NULL;
	}
	return ptr;
}

void* memManagedAllocatePtr(struct memAlloc** mStack, size_t size, void** p_memNode)
{
	void* ptr = EXIP_MALLOC(size);
	if(ptr != NULL)
	{
		struct memAlloc* memNode =  EXIP_MALLOC(sizeof(struct memAlloc));
		if(memNode != NULL)
		{
			memNode->allocation = ptr;
			memNode->nextAlloc = *mStack;
			*mStack = memNode;
			(*p_memNode) = memNode;
		}
		else
			return NULL;
	}
	return ptr;
}

errorCode memManagedReAllocate(void** ptr, size_t size, void* p_memNode)
{
	void* new_ptr = EXIP_REALLOC(*ptr, size);
	if(new_ptr == NULL)
		return MEMORY_ALLOCATION_ERROR;
	*ptr = new_ptr;
	((struct memAlloc*) p_memNode)->allocation = new_ptr;
	return ERR_OK;
}

errorCode freeAllMem(struct memAlloc** mStack)
{
	struct memAlloc* tmpNode;
	while(*mStack != NULL)
	{
		tmpNode = *mStack;
		EXIP_MFREE((*mStack)->allocation);
		*mStack = (*mStack)->nextAlloc;
		EXIP_MFREE(tmpNode);
	}
	return ERR_OK;
}
