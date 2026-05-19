/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under Apache License 2.0            |
|  The full license terms and conditions are located in LICENSE.txt  |
|            Copyright (C) 2024 - 2026, ekrich.org                   |
\===================================================================*/

/**
 * @file schemaLoader.c
 * @brief Shared utilities for loading and parsing EXI schemas
 * @date May 18, 2026
 */

#include "schemaLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

errorCode loadSchemaFile(const char* filename, BinaryBuffer* buffer)
{
	FILE* file = fopen(filename, "rb");
	if (!file)
	{
		fprintf(stderr, "Unable to open file %s\n", filename);
		return EXIP_UNEXPECTED_ERROR;
	}

	// Get file length
	fseek(file, 0, SEEK_END);
	size_t fileLen = ftell(file) + 1;
	fseek(file, 0, SEEK_SET);

	// Allocate and read file contents
	char* fileBuf = (char *) malloc(fileLen);
	if (!fileBuf)
	{
		fprintf(stderr, "Memory allocation error!\n");
		fclose(file);
		return EXIP_MEMORY_ALLOCATION_ERROR;
	}
	fread(fileBuf, fileLen, 1, file);
	fclose(file);

	// Initialize buffer in file mode
	initBinaryBuffer(buffer, fileBuf, fileLen, fileLen, NULL, NULL);

	return EXIP_OK;
}

void freeBinaryBuffers(BinaryBuffer* buffers, unsigned int count)
{
	for (unsigned int i = 0; i < count; i++)
	{
		freeBinaryBuffer(&buffers[i]);
	}
}

errorCode loadSchemaFiles(const char** filenames, unsigned int count,
                           BinaryBuffer* buffers)
{
	errorCode tmp_err_code = EXIP_OK;

	for (unsigned int i = 0; i < count; i++)
	{
		tmp_err_code = loadSchemaFile(filenames[i], &buffers[i]);
		if (tmp_err_code != EXIP_OK)
		{
			freeBinaryBuffers(buffers, i);
			return tmp_err_code;
		}
	}

	return EXIP_OK;
}
