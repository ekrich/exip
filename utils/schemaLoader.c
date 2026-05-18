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

errorCode loadSchemaFiles(char* xsdList,
                          EXIPSchema* schema,
                          EXIOptions* opt)
{
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	FILE *schemaFile;
	BinaryBuffer buffer[MAX_XSD_FILES_COUNT];
	char schemaFileName[FILENAME_MAX];
	unsigned int schemaFilesCount = 0;
	unsigned int i;
	char *token;

	for (token = strtok(xsdList, ","), i = 0; token != NULL; token = strtok(NULL, ","), i++)
	{
		schemaFilesCount++;
		if(schemaFilesCount > MAX_XSD_FILES_COUNT)
		{
			fprintf(stderr, "Too many xsd files given as an input: %d\n", schemaFilesCount);
			return EXIP_UNEXPECTED_ERROR;
		}

		if(strlen(token) < FILENAME_MAX)
		{
			strcpy(schemaFileName, token);
		}
		else
		{
			fprintf(stderr, "Schema filename too long: %zu chars (max %d): %s\n",
				strlen(token), FILENAME_MAX - 1, token);
			return EXIP_UNEXPECTED_ERROR;
		}

		schemaFile = fopen(schemaFileName, "rb");
		if(!schemaFile)
		{
			fprintf(stderr, "Unable to open file %s\n", schemaFileName);
			return EXIP_UNEXPECTED_ERROR;
		}

		// Get file length
		fseek(schemaFile, 0, SEEK_END);
		buffer[i].bufLen = ftell(schemaFile) + 1;
		fseek(schemaFile, 0, SEEK_SET);

		// Allocate memory
		buffer[i].buf = (char *) malloc(buffer[i].bufLen);
		if (!buffer[i].buf)
		{
			fprintf(stderr, "Memory allocation error!\n");
			fclose(schemaFile);
			return EXIP_MEMORY_ALLOCATION_ERROR;
		}

		// Read file contents into buffer
		fread(buffer[i].buf, buffer[i].bufLen, 1, schemaFile);
		fclose(schemaFile);

		buffer[i].bufContent = buffer[i].bufLen;
		buffer[i].ioStrm.readWriteToStream = NULL;
		buffer[i].ioStrm.stream = NULL;
	}

	// Generate the EXI grammars based on the schema information
	tmp_err_code = generateSchemaInformedGrammars(buffer, schemaFilesCount,
	                                               SCHEMA_FORMAT_XSD_EXI, opt, schema, NULL);

	// Free buffers
	for(i = 0; i < schemaFilesCount; i++)
	{
		free(buffer[i].buf);
	}

	return tmp_err_code;
}
