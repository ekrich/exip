/**
 * @defgroup utils EXIP utilities
 *
 * @brief Information on utility tools part of the exip library
 *
 * The folder /utils contains useful utility applications that are used as part of the EXIP library.
 *
 * Utilities include:
 * - exipg (schemaGen/): Grammar generator that creates static representation of EXI grammars from
 *   EXI-encoded XSD files. Used internally to provide grammar definitions for the EXIOptions XML
 *   Schema document.
 * - exipb (codeGen/): Binding generator that creates C structs and encode/decode helper functions
 *   from EXI-encoded XSD schemas.
 * - schemaLoader: Shared utilities for loading and parsing EXI-encoded XSD files, used by both
 *   exipg and exipb.
 *
 * @date Jul 19, 2011
 * @author Rumen Kyusakov
 * @version 0.5
 * @par[Revision] $Id: README.txt 141 2012-06-20 12:21:18Z rumen $
 *
 */
