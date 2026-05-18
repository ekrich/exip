/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under Apache License 2.0            |
|  The full license terms and conditions are located in LICENSE.txt  |
|            Copyright (C) 2024 - 2026, ekrich.org                   |
\===================================================================*/

/**
 * @file schemaLoader.h
 * @brief Shared utilities for loading and parsing EXI schemas
 * @date May 18, 2026
 */

#ifndef SCHEMA_LOADER_H_
#define SCHEMA_LOADER_H_

#include "procTypes.h"
#include "grammarGenerator.h"

#define MAX_XSD_FILES_COUNT 10

/**
 * @brief Load and parse schema files into EXIPSchema
 *
 * Loads one or more EXI-encoded XSD files and generates the complete
 * EXIPSchema with grammars, string tables, and type information.
 *
 * @param[in] xsdList Comma-separated list of .exi schema files
 * @param[out] schema Resulting EXIPSchema structure (caller must call destroySchema)
 * @param[in] opt EXI options for parsing (can be NULL for defaults)
 * @return Error code (EXIP_OK on success)
 */
errorCode loadSchemaFiles(char* xsdList,
                          EXIPSchema* schema,
                          EXIOptions* opt);

#endif /* SCHEMA_LOADER_H_ */
