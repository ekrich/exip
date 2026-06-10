/**
 * @defgroup build_msvs Build using MSVS
 *
 * @brief Information on how to build exip using MS Visual Studio 2022.
 *
 * This build is implemented as a Visual Studio solution (exip.sln) with the following projects:
 *   <ul>
 *     <li>exip.vcxproj - The EXIP library</li>
 *     <li>exipe.vcxproj - An example that encodes a sample EXI stream</li>
 *     <li>exipd.vcxproj - An example that decodes arbitrary EXI streams</li>
 *     <li>exipg.vcxproj - The exipg utility for generating static grammars</li>
 *     <li>exipb.vcxproj - Future Binding Application</li>
 *     <li>check_*.vcxproj - Unit test projects (builtin_grammer, contentio, emptyType, exip, grammar, profile, streamIO, strict_grammer, stringTables, xsi_type)</li>
 *   </ul>
 *
 * <b>Running from Command Line:</b>
 *   <ul>
 *     <li><code>scripts\run-examples.bat</code> - Run example programs (exipe/exipd/exipg)</li>
 *     <li><code>scripts\run-unit-tests.bat [Release|release]</code> - Run all unit tests (defaults to Debug build)</li>
 *   </ul>
 *
 * Compatibility tested with Microsoft C/C++ compiler (MSVC) on Windows 10/11 platforms
 * @note Visual Studio 2022 projects use the C11 standard.
 *
 * To remain portable to MSVC, the following limitation must be observed:
 *   <ul>
 *     <li>Variable Length Arrays (VLA) are not supported. Use sizeof() instead of strlen() for compile-time string size calculations in array declarations. Example: <code>char buf[MAX + sizeof(str) - 1];</code> instead of <code>char buf[MAX + strlen(str)];</code></li>
 *   </ul>
 * @note The MSVS build works since version 0.5.4 using vs2010
 *
 * @date Jun 10, 2026
 * @author Robert Cragie
 * @author Samuel Guilbard
 *
 */
