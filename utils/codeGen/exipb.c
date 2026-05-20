/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under Apache License 2.0            |
|  The full license terms and conditions are located in LICENSE.txt  |
|            Copyright (C) 2024 - 2026, ekrich.org                   |
\===================================================================*/

/**
 * @file exipb.c
 * @brief Command-line utility for generating C bindings from EXI schemas
 * @date May 18, 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include "grammarGenerator.h"
#include "schemaLoader.h"
#include "treeTableSchema.h"
#include "memManagement.h"
#include "dynamicArray.h"

int main(int argc, char *argv[])
{
    (void)argc;  // Unused for now
    (void)argv;  // Unused for now

    printf("exipb - EXI Binding Generator\n");

    // Hardcoded test with example schema
    const char* schemaFiles[] = {
        "examples/simpleDecoding/exipd-test-xsd.exi"
    };
    const unsigned int fileCount = 1;

    BinaryBuffer buffers[1];
    TreeTable* treeT = NULL;
    unsigned int treeTCount = 0;
    SubstituteTable subsTbl = {0};
    EXIPSchema schema;
    EXIOptions opts;
    errorCode err = EXIP_UNEXPECTED_ERROR;

    // Schema files are encoded with Preserve.prefixes
    makeDefaultOpts(&opts);
    SET_PRESERVED(opts.preserve, PRESERVE_PREFIXES);

    printf("Loading schema: %s\n", schemaFiles[0]);

    // Load schema files into buffers
    err = loadSchemaFiles(schemaFiles, fileCount, buffers);
    if (err != EXIP_OK) {
        fprintf(stderr, "Error loading schema files: %d\n", err);
        exit(EXIT_FAILURE);
    }

    printf("Generating optimized TreeTable...\n");

    // Generate optimized TreeTable
    err = generateOptimizedTreeTable(buffers, fileCount, SCHEMA_FORMAT_XSD_EXI, &opts,
                                      &treeT, &treeTCount, &subsTbl, &schema, NULL);

    if (err != EXIP_OK) {
        fprintf(stderr, "Error generating TreeTable: %d\n", err);
        freeBinaryBuffers(buffers, fileCount);
        exit(EXIT_FAILURE);
    }

    printf("Success! Generated %u TreeTable(s)\n", treeTCount);
    printf("Schema has %u namespaces\n", schema.uriTable.count);

    // Cleanup
    for (unsigned int i = 0; i < treeTCount; i++) {
        destroyTreeTable(&treeT[i]);
    }
    EXIP_MFREE(treeT);

    for (unsigned int i = 0; i < subsTbl.count; i++) {
        destroyDynArray(&subsTbl.head[i].dynArray);
    }
    destroyDynArray(&subsTbl.dynArray);

    destroySchema(&schema);
    freeBinaryBuffers(buffers, fileCount);

    printf("Cleanup complete.\n");

    return EXIT_SUCCESS;
}
