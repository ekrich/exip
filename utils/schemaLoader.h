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
 * @brief Load a single schema file into a BinaryBuffer
 *
 * Reads entire file into memory and initializes buffer in file mode
 * (bufLen == bufContent, no streaming).
 *
 * @param[in] filename Path to schema file to load
 * @param[out] buffer BinaryBuffer to initialize (caller must free with freeBinaryBuffer)
 * @return Error code (EXIP_OK on success)
 */
errorCode loadSchemaFile(const char* filename, BinaryBuffer* buffer);

/**
 * @brief Free an array of BinaryBuffers
 *
 * Calls freeBinaryBuffer() on each buffer in the array.
 *
 * @param[in, out] buffers Array of BinaryBuffers to free
 * @param[in] count Number of buffers in the array
 */
void freeBinaryBuffers(BinaryBuffer* buffers, unsigned int count);

/**
 * @brief Load multiple schema files into BinaryBuffer array
 *
 * Takes an array of filenames and loads each into a buffer.
 * Does NOT call generateSchemaInformedGrammars - just loads files.
 *
 * @param[in] filenames Array of schema file paths
 * @param[in] count Number of files to load
 * @param[out] buffers Array to store loaded buffers (caller must free with freeBinaryBuffers)
 * @return Error code (EXIP_OK on success, buffers freed on error)
 */
errorCode loadSchemaFiles(const char** filenames, unsigned int count,
                           BinaryBuffer* buffers);

#endif /* SCHEMA_LOADER_H_ */
