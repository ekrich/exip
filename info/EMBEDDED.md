# Embedded Systems Configuration

EXIP is designed for resource-constrained embedded systems. Key types are configurable to balance functionality vs memory footprint.

## Configurable Types

Define these in your `exipConfig.h` before including EXIP headers:

### EXIP_INDEX - Schema Size

Controls maximum schema table sizes (grammar rules, types, strings, QNames).

```c
#define EXIP_INDEX uint16_t      // Embedded: max 65K entries
#define EXIP_INDEX uint32_t      // Default: max 4B entries (32-bit systems)
#define EXIP_INDEX uint64_t      // Large schemas (rarely needed)
```

**Recommendation:**
- Tiny devices (8KB RAM): `uint16_t` - sufficient for most embedded schemas
- Normal embedded (32KB+ RAM): `uint32_t` - handles large schemas, still compact
- Desktop/server: `uint32_t` (default)

### EXIP_UNSIGNED_INTEGER - Value Range

Controls maximum integer/length values in EXI data.

```c
#define EXIP_UNSIGNED_INTEGER uint32_t    // Embedded: ±2 billion range
#define EXIP_UNSIGNED_INTEGER uint64_t    // Default: full 64-bit range
```

**Recommendation:**
- Embedded with known small values: `uint32_t` - saves memory, handles most real-world data
- General purpose: `uint64_t` (default) - matches EXI spec's unbounded integer support

**Impact:**
- String lengths, binary lengths, array counts limited to this type's max value
- Integer data values limited to this range
- Smaller type = less RAM per value, but overflow risk if data exceeds range

## Example Configurations

### Contiki-NG (Zoul / ARM Cortex-M3)
```c
#define EXIP_INDEX uint16_t
#define EXIP_UNSIGNED_INTEGER uint32_t
```

### Desktop/Server (default)
```c
#define EXIP_INDEX uint32_t
#define EXIP_UNSIGNED_INTEGER uint64_t
```

## Type Promotion and Encoding

When calling EXIP APIs:
- `size_t` → `Index` (at function boundary)
- `Index` → `UnsignedInteger` (for varint encoding)

Example: `serialize.binaryData(strm, data, length)`
- `length` type: `size_t` (from your code)
- Converted to: `Index` (function parameter)
- Encoded as: `UnsignedInteger` (varint on wire)

The actual EXI encoding is the same regardless - only the in-memory representation differs.

## Varint Encoding

EXI uses variable-length encoding - small values use fewer bytes:
- `10` → 1 byte (0x0A)
- `255` → 2 bytes
- Large values → more bytes

So choosing smaller types doesn't affect wire format for small values, only the maximum value range.

## Byte Alignment vs Bit Packing

EXI supports two alignment modes - choose based on your constraints:

### Bit-Packed (Default)
```c
SET_STRICT(opts.enumOpt);  // Default if no alignment specified
```

**Pros:**
- Smallest output size (typically 20-30% smaller than byte-aligned)
- Best for bandwidth-constrained systems (radio, cellular)

**Cons:**
- Slower encoding/decoding (bit shifting operations)
- More CPU cycles and code complexity

### Byte-Aligned
```c
SET_ALIGNMENT(opts.enumOpt, BYTE_ALIGNMENT);
```

**Pros:**
- Faster encoding/decoding (simple byte copies, no bit shifts)
- Less CPU usage
- Simpler implementation

**Cons:**
- Larger output (padding to byte boundaries)
- Wastes bandwidth

### When to Use Each

- **Bit-packed**: Bandwidth is limited, CPU is abundant (cloud, server, desktop)
- **Byte-aligned**: CPU cycles are precious, bandwidth is acceptable (ultra-low-power microcontrollers)

Most embedded systems use **bit-packed** - the space savings outweigh the CPU cost. Only use byte-aligned for devices where CPU is the bottleneck.

## Tradeoffs

| Type | Memory | Max Value | Use Case |
|------|--------|-----------|----------|
| uint16_t | 2 bytes | 65,535 | Tiny schemas, known small data |
| uint32_t | 4 bytes | 4,294,967,295 | Most embedded systems |
| uint64_t | 8 bytes | ~18 quintillion | Full EXI spec compliance |

## See Also

- [procTypes.h](../include/procTypes.h) - Default type definitions
- [exipConfig.h examples](../build/gcc/contiki/exipConfig.h) - Platform-specific configs
