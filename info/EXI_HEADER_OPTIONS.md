# EXI Header Options - Questions and Answers

## Overview
This document explains how the EXI (Efficient XML Interchange) header and options mechanism works in EXIP.

## Question 1: What if the options section is empty?

### Answer
In EXI, you have the `$EXI` header with an options section that can be either present or absent.

**When encoding:**
- Set `strm->header.has_options = false` to indicate no options in header
- The encoder writes a presence bit of `0`
- No options document is written after the version info

**When decoding:**
- The decoder reads the presence bit
- If bit = `0` (no options in header), decoder expects **out-of-band options**
- Out-of-band options must be set programmatically before decoding
- If no out-of-band options exist, decoder returns `EXIP_HEADER_OPTIONS_MISMATCH` error

**Valid scenarios:**
1. Options in header (presence bit = 1) → use those options
2. No options in header (presence bit = 0) + out-of-band options set → use out-of-band options
3. No options in header (presence bit = 0) + no out-of-band options → **ERROR**

### Code References
- Encoding: [headerEncode.c:53](../src/contentIO/src/headerEncode.c#L53)
- Decoding: [headerDecode.c:92-118](../src/contentIO/src/headerDecode.c#L92-L118)

---

## Question 2: How does it know if default or header options are used?

### Answer
The **presence bit** comes immediately after the distinguishing bits and signals what to expect:

#### EXI Header Bit Sequence:
```
1. [Optional] $EXI cookie (32 bits: $, E, X, I)
2. Distinguishing Bits: "10" (2 bits)
3. Presence Bit for Options: 0 or 1 (1 bit) ← THIS TELLS YOU!
4. Preview version flag (1 bit)
5. Version number (4+ bits, typically 4 for version 1)
6. [Conditional] If presence bit = 1: EXI Options document
7. [Then] Actual EXI document body starts
```

#### Decision Logic:

**Presence bit = 0:**
- "No options in header, use out-of-band/default"
- Decoder checks if out-of-band options were set programmatically
- If yes → use those
- If no → **ERROR**: `EXIP_HEADER_OPTIONS_MISMATCH`

**Presence bit = 1:**
- "Options document follows immediately"
- Decoder parses the EXI-encoded options document
- These options override any out-of-band settings

### Code References
- Encoding flow: [headerEncode.c:52-69](../src/contentIO/src/headerEncode.c#L52-L69)
- Decoding flow: [headerDecode.c:92-118](../src/contentIO/src/headerDecode.c#L92-L118)

---

## Question 3: If presence bit = 1, what signals defaults vs specified? How many header bits are there?

### Answer

#### How Defaults vs Specified Works

When presence bit = 1, an **EXI-encoded options document** follows. This document uses **selective encoding**:
- **Only non-default values are included**
- **Missing elements = default values**
- **Present elements = explicitly specified values**

#### Options Document Structure
```xml
<header>
  [<lesscommon>               ← Only if any lesscommon option differs from default
    [<uncommon>               ← Only if any uncommon option differs from default
      [<alignment>...]        ← Only if not BIT_PACKED
      [<selfContained/>]      ← Only if true
      [<valueMaxLength>...]   ← Only if not INDEX_MAX
      [<valuePartitionCapacity>...] ← Only if not INDEX_MAX
      [<datatypeRepresentationMap>...] ← Only if present
    </uncommon>]
    [<preserve>               ← Only if preserve != 0
      [<dtd/>]                ← Only if PRESERVE_DTD set
      [<prefixes/>]           ← Only if PRESERVE_PREFIXES set
      [<lexicalValues/>]      ← Only if PRESERVE_LEXVALUES set
      [<comments/>]           ← Only if PRESERVE_COMMENTS set
      [<pis/>]                ← Only if PRESERVE_PIS set
    </preserve>]
    [<blockSize>...]          ← Only if not 1000000
  </lesscommon>]
  [<common>                   ← Only if any common option differs from default
    [<compression/>]          ← Only if compression enabled
    [<fragment/>]             ← Only if fragment enabled
    [<schemaId>...]           ← Only if schema ID present
  </common>]
  [<strict/>]                 ← Only if strict mode enabled
</header>
```

#### Default Values
- Alignment: `BIT_PACKED`
- Self-contained: `false`
- valueMaxLength: `INDEX_MAX`
- valuePartitionCapacity: `INDEX_MAX`
- preserve: `0` (nothing preserved)
- blockSize: `1000000`
- compression: `false`
- fragment: `false`
- schemaID: `SCHEMA_ID_ABSENT`
- strict: `false`

#### Example: All Defaults
If all options are default, the document is simply:
```xml
<header></header>
```
Encoded in strict EXI, this is approximately **3-5 bits**.

### Total Header Bits

#### Fixed Header Bits (before options):
1. **Cookie (optional)**: 32 bits (4 bytes) - `$EXI`
2. **Distinguishing Bits**: 2 bits - `10`
3. **Presence Bit**: 1 bit
4. **Preview Version**: 1 bit
5. **Version Number**: minimum 4 bits (for version 1)

#### Minimum Sizes:
- **No cookie, no options**: 2 + 1 + 1 + 4 = **8 bits (1 byte)**
- **With cookie, no options**: 32 + 2 + 1 + 1 + 4 = **40 bits (5 bytes)**
- **No cookie, all-default options**: 8 + ~5 = **~13 bits**
- **With cookie, all-default options**: 40 + ~5 = **~45 bits**

#### Variable Size:
- **With specified options**: Add bits based on which options differ from defaults
- Options document is itself EXI-encoded (very compact)
- Each non-default option adds a few bits for element tags + value encoding

### Code References
- Default detection: [headerEncode.c:138-148](../src/contentIO/src/headerEncode.c#L138-L148)
- Options serialization: [headerEncode.c:123-437](../src/contentIO/src/headerEncode.c#L123-L437)
- Options parsing: [headerDecode.c:136-194](../src/contentIO/src/headerDecode.c#L136-L194)

---

---

## Question 4: How does the processor distinguish between empty options document and the start of the actual document?

### Answer

The key is that the **options document is itself a complete EXI document** with explicit start and end markers.

#### The Options Parser

When presence bit = 1, the decoder creates a separate parser specifically for the options document:

1. **Schema-driven parsing** ([headerDecode.c:167](../src/contentIO/src/headerDecode.c#L167))
   - Uses the predefined `ops_schema` (EXI Options schema)
   - The schema defines the structure: `<header>...</header>`

2. **Parse until completion** ([headerDecode.c:170-173](../src/contentIO/src/headerDecode.c#L170-L173))
   ```c
   while(tmp_err_code == EXIP_OK)
   {
       tmp_err_code = parseNext(&optionsParser);
   }
   ```
   - Parser processes EXI events: startDocument, startElement, endElement, endDocument
   - Returns `EXIP_PARSING_COMPLETE` when it encounters the **endDocument** event

3. **Position tracking** ([headerDecode.c:180-182](../src/contentIO/src/headerDecode.c#L180-L182))
   ```c
   strm->context.bitPointer = optionsParser.strm.context.bitPointer;
   strm->context.bufferIndx = optionsParser.strm.context.bufferIndx;
   ```
   - After parsing completes, the bit position points to where the actual document starts
   - No ambiguity - the parser consumed exactly the options document

#### Options Document Structure

The options document is a **complete EXI-encoded document** with explicit events:

**Empty options (all defaults):**
```
[implicit] startDocument       (0 bits)
startElement <header>          (1 bit)  ← EXPLICIT EVENT: event code "0"
endElement </header>           (2 bits) ← EXPLICIT EVENT: event code "11" (part 3)
endDocument                    (0 bits) ← IMPLICIT (no bits written)
```
**Total: Exactly 3 bits**

**Options with values (example with strict mode):**
```
[implicit] startDocument       (0 bits)
startElement <header>          (1 bit)  ← event code "0"
  startElement <strict/>       (1 bit)  ← event code "0" (strict chosen)
  endElement </strict>         (0 bits) ← implicit (no content in strict)
endElement </header>           (0 bits) ← implicit (all required content present)
endDocument                    (0 bits) ← implicit
```
**Total: 2 bits for strict-only options**

#### How Events Are Encoded

From [headerEncode.c:132-434](../src/contentIO/src/headerEncode.c#L132-L434):

1. **Line 132**: startDocument is implicit (0 bits)
2. **Line 135-136**: `serializeEvent()` writes **1 bit** for startElement `<header>` (event code 0)
3. **Lines 154-424**: Conditional child elements (only non-defaults, variable bits)
4. **Line 428-429**: `serializeEvent()` writes endElement `</header>` event (variable bits based on content)
5. **Line 433-434**: `serializeEvent()` writes endDocument event (**0 bits** - implicit!)

#### Event Code Structure

Each event is encoded using `tmpEvCode`:
- `tmpEvCode.bits[0]` = **exact number of bits** to write
- `tmpEvCode.part[0]` = **value** to write in those bits
- Example: `bits[0]=1, part[0]=0` → write "0" in 1 bit
- Example: `bits[0]=2, part[0]=3` → write "11" in 2 bits

Both cases end with the **endDocument event**, which is the definitive signal that:
- The options document is complete
- The current bit position is where the actual EXI document body begins
- Each event is explicitly encoded as EXI grammar productions

#### Why This Works

1. **Complete EXI document structure** - Options document has startDocument and endDocument
2. **Schema validation** - Parser knows the expected structure from `ops_schema`
3. **Event-driven parsing** - Each EXI event is explicit (startElement, endElement, etc.)
4. **No ambiguity** - The `endDocument` event clearly marks the boundary
5. **Bit position preserved** - Parser tracks exactly where it stopped reading

#### Schema-Informed Encoding

The options document uses **schema-informed EXI encoding** with a pre-compiled schema:

- **Built-in schema** ([staticEXIOptions.c](../src/contentIO/src/staticEXIOptions.c)): Contains pre-defined element names
  - `ops_LN_4_20`: "header"
  - `ops_LN_4_33`: "strict"
  - `ops_LN_4_6`: "common"
  - `ops_LN_4_25`: "lesscommon"
  - `ops_LN_4_36`: "uncommon"
  - All in namespace: `http://www.w3.org/2009/exi`

- **No string transmission**: Element names are NOT written to the stream
  - Event codes reference schema grammar positions
  - Both encoder and decoder have the same `ops_schema` compiled in
  - Element name "header" → just an event code (grammar production index)

- **Why it's so compact**: Schema-informed encoding means names are implicit
  - Only event codes (grammar productions) are written
  - Decoder resolves event codes to element names via schema lookup

### Code References
- Options parsing: [headerDecode.c:136-194](../src/contentIO/src/headerDecode.c#L136-L194)
- Parse loop: [headerDecode.c:170-173](../src/contentIO/src/headerDecode.c#L170-L173)
- Completion check: [headerDecode.c:177-178](../src/contentIO/src/headerDecode.c#L177-L178)

---

## Summary

1. **Presence bit is explicit** - no ambiguity about whether options follow
2. **When present, defaults are implicit** - only non-default values are encoded
3. **Minimum overhead** - 8 bits for basic header, ~5 more bits for all-default options
4. **Out-of-band options are supported** - useful when both sides pre-agree on configuration
5. **Error on missing options** - decoder requires either in-band or out-of-band options
6. **Options boundary is explicit** - endDocument event clearly marks where actual document begins

## Related Files
- [headerEncode.c](../src/contentIO/src/headerEncode.c) - Header encoding implementation
- [headerDecode.c](../src/contentIO/src/headerDecode.c) - Header decoding implementation
- [procTypes.h](../include/procTypes.h) - EXIheader structure definition
- [EXISerializer.h](../include/EXISerializer.h) - Serializer interface
