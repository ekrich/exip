/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file exipg.c
 * @brief Command-line utility for generating EXI grammars from XML schema definition
 *
 * @date Nov 5, 2012
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#include "createGrammars.h"
#include "grammarGenerator.h"
#include "schemaLoader.h"

#define OUT_EXIP     0
#define OUT_TEXT     1
#define OUT_SRC_DYN  2
#define OUT_SRC_STAT 3

static void printfHelp();
static bool validateAndParseUInt(const char *str, char **endPtr, unsigned int *result, char expectedDelim);

int main(int argc, char *argv[])
{
	// options
    const char *opt_help = "-help";
	const char *opt_exip = "-exip";
	const char *opt_text = "-text";
	const char *opt_dynamic = "-dynamic";
	const char *opt_static = "-static";
	const char *opt_pfx = "-pfx";
	const char *opt_ops = "-ops";
	const char *opt_schema = "-schema";

	FILE *outfile = stdout; // Default is the standard output
	EXIPSchema schema = {0};
	unsigned char outputFormat = OUT_EXIP;
	int argIndex = 1;
	errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;
	char prefix[20] = "prfx_"; // The default prefix
	EXIOptions maskOpt;
	Deviations dvis = {0};
	char *schemaFiles = NULL;

	makeDefaultOpts(&maskOpt);

	if(argc == 1)
	{
		printfHelp();
		return 0;
	}

	if(strcmp(argv[argIndex], opt_help) == 0)
	{
		printfHelp();
		return 0;
	}
	else if(strcmp(argv[argIndex], opt_exip) == 0)
	{
		outputFormat = OUT_EXIP;
		argIndex++;
	}
	else if(strcmp(argv[argIndex], opt_text) == 0)
	{
		outputFormat = OUT_TEXT;
		argIndex++;
	}
	else if(strcmp(argv[argIndex], opt_dynamic) == 0)
	{
		outputFormat = OUT_SRC_DYN;
		argIndex++;
	}
	else if(strncmp(argv[argIndex], opt_static, strlen(opt_static)) == 0)
	{
		outputFormat = OUT_SRC_STAT;

        if(argv[argIndex][strlen(opt_static)] == '=')
        {
            char *pEnd;
		    char *deviations = argv[argIndex] + strlen(opt_static) + 1; // 1 for =

            if(!validateAndParseUInt(deviations, &pEnd, &dvis.url, ':') ||
                !validateAndParseUInt(pEnd + 1, &pEnd, &dvis.ln, ':') ||
                !validateAndParseUInt(pEnd + 1, &pEnd, &dvis.pfx, ':') ||
                !validateAndParseUInt(pEnd + 1, &pEnd, &dvis.grammar, '\0'))  // ends with null
            {
                fprintf(stderr, "Invalid deviations format: %s\n", argv[argIndex]);
                fprintf(stderr, "Expected: -static=<url>:<ln>:<pfx>:<grammars>\n");
                exit(1);
            }
        }
		argIndex++;
	}

	if(argc <= argIndex)
	{
		printfHelp();
		return 0;
	}

	if(strncmp(argv[argIndex], opt_pfx, strlen(opt_pfx)) == 0)
	{
		char *pfxValue = argv[argIndex] + strlen(opt_pfx);

		if(*pfxValue == '=')
		{
			pfxValue++;  // skip the '='
			strcpy(prefix, pfxValue);
		}
		else
		{
			// -pfx without = or -pfxXXX
			fprintf(stderr, "Invalid argument: %s\n", argv[argIndex]);
			fprintf(stderr, "Expected: %s=<prefix>\n", opt_pfx);
			printfHelp();
			exit(1);
		}

		argIndex++;
	}

	if(argc <= argIndex)
	{
		printfHelp();
		return 0;
	}

	if(strncmp(argv[argIndex], opt_ops, strlen(opt_ops)) == 0)
	{
		char *opsValue = argv[argIndex] + strlen(opt_ops);

		if(*opsValue == '=')
		{
			opsValue++;  // skip the '='
			if(opsValue[0] == '1')
				SET_STRICT(maskOpt.enumOpt);

			if(opsValue[1] == '1')
				SET_SELF_CONTAINED(maskOpt.enumOpt);

			if(opsValue[2] == '1')
				SET_PRESERVED(maskOpt.preserve, PRESERVE_DTD);

			if(opsValue[3] == '1')
				SET_PRESERVED(maskOpt.preserve, PRESERVE_PREFIXES);

			if(opsValue[4] == '1')
				SET_PRESERVED(maskOpt.preserve, PRESERVE_LEXVALUES);

			if(opsValue[5] == '1')
				SET_PRESERVED(maskOpt.preserve, PRESERVE_COMMENTS);

			if(opsValue[6] == '1')
				SET_PRESERVED(maskOpt.preserve, PRESERVE_PIS);
		}
		else
		{
			// -ops without = or -opsXXX
			fprintf(stderr, "Invalid argument: %s\n", argv[argIndex]);
			fprintf(stderr, "Expected: %s=<ops_mask>\n", opt_ops);
			printfHelp();
			exit(1);
		}

		argIndex++;
	}

	if(argc <= argIndex)
	{
		fprintf(stderr, "Missing required %s=<xsd_in> argument\n", opt_schema);
		printfHelp();
		return 0;
	}

	if(strncmp(argv[argIndex], opt_schema, strlen(opt_schema)) == 0)
    {
        char *xsdList = argv[argIndex] + strlen(opt_schema);

        if(*xsdList == '=')
        {
            xsdList++;  // skip the '='
            schemaFiles = xsdList;
        }
        else
        {
            // -schema without = or -schemaXXX
            fprintf(stderr, "Invalid argument: %s\n", argv[argIndex]);
            fprintf(stderr, "Expected: %s=<xsd_in>\n", opt_schema);
            printfHelp();
            exit(1);
        }

        argIndex += 1;
    }

	if(argc > argIndex)
	{
		outfile = fopen(argv[argIndex], "wb" );
		if(!outfile)
		{
			fprintf(stderr, "Unable to open file %s", argv[argIndex]);
			return 1;
		}
		argIndex += 1;
	}

	if(schemaFiles == NULL)
	{
		fprintf(stderr, "Missing required %s=<xsd_in> argument\n", opt_schema);
		printfHelp();
		return 1;
	}

	// Parse schema files and output now that all options are processed
	{
		BinaryBuffer buffer[MAX_XSD_FILES_COUNT];
		char schemaFileName[FILENAME_MAX];
		unsigned int count = 0;
		unsigned int i;
		char *token;
		const char* filenames[MAX_XSD_FILES_COUNT];

		for (token = strtok(schemaFiles, ","), i = 0; token != NULL; token = strtok(NULL, ","), i++)
		{
			count++;
			if(count > MAX_XSD_FILES_COUNT)
			{
				fprintf(stderr, "Too many xsd files given as an input: %d\n", count);
				exit(1);
			}

			if(strlen(token) < FILENAME_MAX)
			{
				strcpy(schemaFileName, token);
				filenames[i] = token;
			}
			else
			{
				fprintf(stderr, "Schema filename too long: %zu chars (max %d): %s\n",
					strlen(token), FILENAME_MAX - 1, token);
				exit(1);
			}
		}

		tmp_err_code = loadSchemaFiles(filenames, count, buffer);
		if(tmp_err_code != EXIP_OK)
		{
			printf("\nError loading schema files: %d", tmp_err_code);
			exit(1);
		}

		tmp_err_code = generateSchemaInformedGrammars(buffer, count,
		                                               SCHEMA_FORMAT_XSD_EXI, &maskOpt, &schema, NULL);

		freeBinaryBuffers(buffer, count);

		if(tmp_err_code != EXIP_OK)
		{
			printf("\nGrammar generation error occurred: %d", tmp_err_code);
			exit(1);
		}
	}

	switch(outputFormat)
	{
		case OUT_TEXT:
			tmp_err_code = toText(&schema, outfile);
		break;
		case OUT_EXIP:
			tmp_err_code = toEXIP(&schema, outfile);
		break;
		case OUT_SRC_STAT:
			tmp_err_code = toStaticSrc(&schema, prefix, outfile, dvis);
		break;
		case OUT_SRC_DYN:
			tmp_err_code = toDynSrc(&schema, outfile);
		break;
		default:
			printf("\nUnsupported output format!");
			exit(1);
		break;
	}

	destroySchema(&schema);

	fclose(outfile);

	if(tmp_err_code != EXIP_OK)
	{
		printf("\nError during grammar output!");
		exit(1);
	}

	return 0;
}

static void printfHelp()
{
    printf("\n" );
    printf("  EXIP     Copyright (c) 2010 - 2012, EISLAB - Luleå University of Technology Version 0.5.1 \n");
    printf("           Author: Rumen Kyusakov\n");
    printf("  Usage:   exipg [options] -schema=<xsd_in> [grammar_out] \n\n");
    printf("           Options: [-help | [[-exip | -text | -dynamic | -static[=<deviations>]] [-pfx=<prefix>] [-ops=<ops_mask>]] ] \n");
    printf("           -help        :   Prints this help message\n");
    printf("           -exip        :   Format the output schema definitions in EXIP-specific format (Default)\n");
    printf("           -text        :   Format the output schema definitions in human readable text format\n");
    printf("           -dynamic     :   Create C code for the grammars defined. The output is a C function that dynamically generates the grammars\n");
    printf("           -static      :   Create C code for the grammars defined. The output is C structures describing the grammars\n");
    printf("           deviations   :   When static C code is chosen for the output, this defines a static size of the possible extensions\n");
    printf("                            for URI, Local names, prefixes, and build-in grammars. The format is: <uri>:<ln>:<pfx>:<grammars>\n");
    printf("                            For example deviations -static=2:5:1:10 will allow for two non-schema namespaces, 5 new local names\n");
    printf("                            per each schema namespace, 1 additional prefix per namespace, and 10 new built-in grammars.\n");
    printf("                            The larger deviations the more memory is required.\n");
    printf("           -pfx         :   When in -dynamic or -static mode, this option allows you to specify a unique prefix for the\n");
    printf("                            generated global types. The default is \"prfx_\"\n");
    printf("           ops_mask     :   The format is: <STRICT><SELF_CONTAINED><dtd><prefixes><lexicalValues><comments><pis> := <0|1><0|1><0|1><0|1><0|1><0|1><0|1>\n");
    printf("                            Use this argument only for specifying out-of-band options. That is if no options are specified in the header of the <xsd_in>\n");
    printf("                            EXI encoded schema files but some options are used during encoding.\n");
    printf("                            This argument is useful for generating the \"EXI Options\" grammar where STRICT is set and the rest are default options. \n");
    printf("                            In this way the bootstrapping of the code is easier. The mask to use for EXIOptions-xsd.exi is -ops=0001000 \n");
    printf("           -schema      :   The source schema definitions - all referenced schema files should be included in <xsd_in>\n");
    printf("           xsd_in       :   Comma-separated list of schema documents encoded in EXI with Preserve.prefixes. The first schema is the main one and\n");
    printf("                            the rest are schemas that are referenced from the main one through the <xs:import> statement.\n");
    printf("           grammar_out  :   Destination file for the grammar output (Default is the standard output) \n\n");
    printf("  Purpose: Manipulation of EXIP schemas\n");
    printf("\n" );
}

static bool validateAndParseUInt(const char *str, char **endPtr, unsigned int *result, char expectedDelim)
{
   *result = (unsigned int) strtoul(str, endPtr, 10);
    if(**endPtr != expectedDelim)
    {
        return false;
    }
    else
    {
        return true;
    }
}
