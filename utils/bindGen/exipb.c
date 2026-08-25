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
#include "stringManipulate.h"
#include "errorHandle.h"

#if DEBUG_BINDING == ON
// Simple tree printer for development (doesn't use DEBUG_MSG)
static void printTreeEntry(TreeTableEntry* entry, int depth, const char* prefix)
{
    if (entry == NULL) return;

    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    // Print prefix and element type
    printf("%s[%s]", prefix, elemStrings[entry->element]);

    // Print all relevant attributes
    if (entry->attributePointers[ATTRIBUTE_NAME].str != NULL) {
        printf(" name=\"");
        printString(&entry->attributePointers[ATTRIBUTE_NAME]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_TYPE].str != NULL) {
        printf(" type=\"");
        printString(&entry->attributePointers[ATTRIBUTE_TYPE]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_REF].str != NULL) {
        printf(" ref=\"");
        printString(&entry->attributePointers[ATTRIBUTE_REF]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_BASE].str != NULL) {
        printf(" base=\"");
        printString(&entry->attributePointers[ATTRIBUTE_BASE]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_MIN_OCCURS].str != NULL) {
        printf(" minOccurs=\"");
        printString(&entry->attributePointers[ATTRIBUTE_MIN_OCCURS]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_MAX_OCCURS].str != NULL) {
        printf(" maxOccurs=\"");
        printString(&entry->attributePointers[ATTRIBUTE_MAX_OCCURS]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_USE].str != NULL) {
        printf(" use=\"");
        printString(&entry->attributePointers[ATTRIBUTE_USE]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_NILLABLE].str != NULL) {
        printf(" nillable=\"");
        printString(&entry->attributePointers[ATTRIBUTE_NILLABLE]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_FORM].str != NULL) {
        printf(" form=\"");
        printString(&entry->attributePointers[ATTRIBUTE_FORM]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_NAMESPACE].str != NULL) {
        printf(" namespace=\"");
        printString(&entry->attributePointers[ATTRIBUTE_NAMESPACE]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_MIXED].str != NULL) {
        printf(" mixed=\"");
        printString(&entry->attributePointers[ATTRIBUTE_MIXED]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_ABSTRACT].str != NULL) {
        printf(" abstract=\"");
        printString(&entry->attributePointers[ATTRIBUTE_ABSTRACT]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_SUBSTITUTION_GROUP].str != NULL) {
        printf(" substitutionGroup=\"");
        printString(&entry->attributePointers[ATTRIBUTE_SUBSTITUTION_GROUP]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_VALUE].str != NULL) {
        printf(" value=\"");
        printString(&entry->attributePointers[ATTRIBUTE_VALUE]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_ITEM_TYPE].str != NULL) {
        printf(" itemType=\"");
        printString(&entry->attributePointers[ATTRIBUTE_ITEM_TYPE]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_MEMBER_TYPES].str != NULL) {
        printf(" memberTypes=\"");
        printString(&entry->attributePointers[ATTRIBUTE_MEMBER_TYPES]);
        printf("\"");
    }
    if (entry->attributePointers[ATTRIBUTE_PROC_CONTENTS].str != NULL) {
        printf(" processContents=\"");
        printString(&entry->attributePointers[ATTRIBUTE_PROC_CONTENTS]);
        printf("\"");
    }
    printf("\n");

    // Print supertype (indented more)
    if (entry->supertype.entry != NULL) {
        printTreeEntry(entry->supertype.entry, depth + 2, "[super] ");
    }

    // Print child (same level as parent)
    if (entry->child.entry != NULL) {
        printTreeEntry(entry->child.entry, depth + 1, "");
    }

    // Print next sibling
    if (entry->next != NULL) {
        printTreeEntry(entry->next, depth, "");
    }
}
#endif

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
    printf("Schema has %zu namespaces\n", schema.uriTable.count);

#if DEBUG_BINDING == ON
    // Print tree table structure for inspection (debug output only)
    DEBUG_MSG(INFO, DEBUG_BINDING, ("\n=== Tree Table Structure ===\n"));
    for (unsigned int i = 0; i < treeTCount; i++) {
        DEBUG_MSG(INFO, DEBUG_BINDING, ("\n--- TreeTable %u (entries: %zu) ---\n", i, treeT[i].count));
        for (Index j = 0; j < treeT[i].count; j++) {
            DEBUG_MSG(INFO, DEBUG_BINDING, ("\nEntry %zu:\n", j));
            printTreeEntry(&treeT[i].tree[j], 0, "");
        }
    }
    DEBUG_MSG(INFO, DEBUG_BINDING, ("\n=== End Tree Table Structure ===\n\n"));
#endif

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
