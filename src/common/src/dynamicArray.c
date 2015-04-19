/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file dynamicArray.c
 * @brief Implementation for untyped dynamic array
 *
 * @date Jan 25, 2011
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#include "dynamicArray.h"
#include "memManagement.h"

/**
 * Used for fixing portability issue due to padding in DynArray derived structures such as
 *  struct [ConcreteDynamicArray]
 *   {
 *     DynArray dynArray;
 *     [ArrayEntryType]* base;
 *     Index count;
 *   };
 *   The fix is contributed by John Fisher (see https://sourceforge.net/p/exip/discussion/Q%26A/thread/ac602d44/#ae3c)
 */
typedef struct
{
    DynArray dynArray;
    void* base;
    Index count;
} OuterDynamicArray;

errorCode createDynArray(DynArray* dynArray, size_t entrySize, uint16_t chunkEntries)
{
	void** base = &((OuterDynamicArray *)dynArray)->base;
	Index* count = &((OuterDynamicArray *)dynArray)->count;

	*base = EXIP_MALLOC(entrySize*chunkEntries);
	if(*base == NULL)
		return EXIP_MEMORY_ALLOCATION_ERROR;

	dynArray->entrySize = entrySize;
	*count = 0;
	dynArray->chunkEntries = chunkEntries;
	dynArray->arrayEntries = chunkEntries;

	return EXIP_OK;
}

errorCode addEmptyDynEntry(DynArray* dynArray, void** entry, Index* entryID)
{
	void** base;
	Index* count;

	if(dynArray == NULL)
		return EXIP_NULL_POINTER_REF;

	base = &((OuterDynamicArray *)dynArray)->base;
	count = &((OuterDynamicArray *)dynArray)->count;
	if(dynArray->arrayEntries == *count)   // The dynamic array must be extended first
	{
		size_t addedEntries;

		addedEntries = (dynArray->chunkEntries == 0)?DEFAULT_NUMBER_CHUNK_ENTRIES:dynArray->chunkEntries;

		if(*base == NULL)
		{
			*base = EXIP_MALLOC(dynArray->entrySize * addedEntries);
			if(*base == NULL)
				return EXIP_MEMORY_ALLOCATION_ERROR;
		}
		else
		{
			*base = EXIP_REALLOC(*base, dynArray->entrySize * (*count + addedEntries));
			if(*base == NULL)
				return EXIP_MEMORY_ALLOCATION_ERROR;
		}

		dynArray->arrayEntries = dynArray->arrayEntries + addedEntries;
	}

	*entry = (void*)((unsigned char *)(*base) + (*count * dynArray->entrySize));

	*entryID = *count;

	*count += 1;
	return EXIP_OK;
}

errorCode addDynEntry(DynArray* dynArray, void* entry, Index* entryID)
{
	errorCode tmp_err_code;
	void *emptyEntry;

	TRY(addEmptyDynEntry(dynArray, &emptyEntry, entryID));

	memcpy(emptyEntry, entry, dynArray->entrySize);
	return EXIP_OK;
}

errorCode delDynEntry(DynArray* dynArray, Index entryID)
{
	void** base;
	Index* count;

	if(dynArray == NULL)
		return EXIP_NULL_POINTER_REF;

	base = &((OuterDynamicArray *)dynArray)->base;
	count = &((OuterDynamicArray *)dynArray)->count;

	if(entryID == *count - 1)
	{
		*count -= 1;
	}
	else if(*count - 1 - entryID >= 0)
	{
		/* Shuffle the array down to fill the removed entry */
		memcpy(((unsigned char *)*base) + entryID * dynArray->entrySize,
			   ((unsigned char *)*base) + entryID * dynArray->entrySize + dynArray->entrySize,
			   (*count - 1 - entryID) * dynArray->entrySize);
		*count -= 1;
	}
	else
		return EXIP_OUT_OF_BOUND_BUFFER;

	return EXIP_OK;
}

void destroyDynArray(DynArray* dynArray)
{
	void** base = &((OuterDynamicArray *)dynArray)->base;
	EXIP_MFREE(*base);
}
