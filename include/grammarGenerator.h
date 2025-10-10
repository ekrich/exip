/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          This work is licensed under BSD 3-Clause License          |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/

/**
 * @file grammarGenerator.h
 * @brief Definition and functions for generating Schema-informed Grammar definitions
 * @date Nov 22, 2010
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id$
 */

#ifndef GRAMMARGENERATOR_H_
#define GRAMMARGENERATOR_H_

#include "errorHandle.h"
#include "procTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name Supported schema formats
 * Such as XML-XSD, EXI-XSD, DTD or any other schema representation
 */
enum SchemaFormat
{
	SCHEMA_FORMAT_XSD_EXI   = 0,
	SCHEMA_FORMAT_XSD_XML   = 1,
	SCHEMA_FORMAT_DTD       = 2,
	SCHEMA_FORMAT_RELAX_NG  = 3
};

typedef enum SchemaFormat SchemaFormat;

/**
 * @brief Call-back handler, to be implemented by the applications
 * It is called to resolve references to schemas (&lt;include&gt; and &lt;import&gt;)
 * not included in the BinaryBuffer list of generateSchemaInformedGrammars().
 *
 * @param[in] namespace NULL for &lt;include&gt; reference; the target namespace of &lt;import&gt;-ed schema otherwise
 * @param[in] schemaLocation can be NULL for &lt;import&gt; reference only; The uri location of the schema otherwise
 * @param[out] buffers an array of input buffers holding the representation of the schema &lt;include&gt;-ed or &lt;import&gt;-ed
 *             Each buffer refers to one schema file.
 * @param[out] bufCount the number of buffers in the array
 * @param[out] schemaFormat EXI, XSD, DTD or any other schema representation supported
 * @param[out] opt options used for EXI schemaFormat - otherwise NULL. If options are set then they will be used
 * for processing the EXI XSD stream although no options are specified in the EXI header. If there are
 * options defined in the EXI header of the XSD stream then this parameter must be NULL.
 * @return Error handling code
 *
 * @note In the case of &lt;include&gt; - both the host- and included-schema must have identical target namespace.
 * That is, the current implementation does not support the case of using &lt;include&gt; from XML Schema
 * that has non empty target namespace and the referenced/included XML schema have an empty target
 * namespace, although this is allowed by the XML Schema spec.
 */
errorCode loadSchemaHandler(String* namespace, String* schemaLocation, BinaryBuffer** buffers, unsigned int* bufCount, SchemaFormat* schemaFormat, EXIOptions** opt);

/**
 * @brief Given a set of XML schemes, generates all Schema-informed Element and Type Grammars
 * as well as the pre-populated string tables (i.e. EXIPSchema instance)
 * The current version supports only SCHEMA_FORMAT_XSD_EXI format of the XML schema.
 *
 * @param[in] buffers an array of input buffers holding the representation of the schema.
 * 			  Each buffer refers to one schema file.
 * @param[in] bufCount the number of buffers in the array
 * @param[in] schemaFormat EXI, XSD, DTD or any other schema representation supported
 * @param[in] opt options used for EXI schemaFormat - otherwise NULL. If options are set then they will be used
 * for processing the EXI XSD stream although no options are specified in the EXI header. If there are
 * options defined in the EXI header of the XSD stream then this parameter must be NULL.
 * @param[out] schema the resulted schema information used for processing EXI streams
 * @param[in] loadSchemaHandler Call-back handler for loading &lt;include&gt;-ed or &lt;import&gt;-ed schema files; Can be left NULL
 * if no &lt;include&gt; or &lt;import&gt; statements are used in the XML schema.
 * @return Error handling code
 *
 * @note In the case of &lt;include&gt; - both the host- and included-schema must have identical target namespace.
 * That is, the current implementation does not support the case of using &lt;include&gt; from XML Schema
 * that has non empty target namespace and the referenced/included XML schema have an empty target
 * namespace, although this is allowed by the XML Schema spec.
 */
errorCode generateSchemaInformedGrammars(BinaryBuffer* buffers, unsigned int bufCount, SchemaFormat schemaFormat, EXIOptions* opt, EXIPSchema* schema,
		errorCode (*loadSchemaHandler) (String* namespace, String* schemaLocation, BinaryBuffer** buffers, unsigned int* bufCount, SchemaFormat* schemaFormat, EXIOptions** opt));

/**
 * @brief Frees all the memory allocated by an EXIPSchema object
 * @param[in] schema the schema containing the EXI grammars to be freed
 */
void destroySchema(EXIPSchema* schema);

#ifdef __cplusplus
}
#endif

#endif /* GRAMMARGENERATOR_H_ */
