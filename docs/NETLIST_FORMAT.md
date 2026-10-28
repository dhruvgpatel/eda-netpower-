# netpower: Structural Verilog Subset Specification

`netpower` parses a strict, clearly defined, scalar structural subset of Verilog HDL (IEEE 1364).

## Supported Grammar

### 1. Module Definition
- Exactly one module definition per file.
- Non-ANSI port list:
```verilog
module <module_name> (<port_name>, <port_name>, ...);
```
- Closed by `endmodule`. No tokens may follow `endmodule`.

### 2. Signal Declarations
- Scalar port and wire declarations:
```verilog
input a, b;
output sum, carry;
wire n1, n2, n3;
```
- Rules:
  - Every port listed in the module header must be declared as `input` or `output`.
  - Every declared `input` or `output` must appear in the module header.
  - `wire` nets represent internal signals and must **not** appear in the module header.
  - Duplicate declarations of the same net are rejected, except when declaring a net as both `input` and `output` to represent a direct feedthrough / alias (`INPUT_OUTPUT`).

### 3. Primitive Gate Instantiations
- Positional gate instantiation:
```verilog
<primitive> <instance_name> (<output_net>, <input_1>, <input_2>, ...);
```
- Output net is always the first terminal.
- Gate primitives and arity:
  - `not`, `buf`: exactly 1 input (2 terminals total).
  - `and`, `or`, `nand`, `nor`, `xor`, `xnor`: 2 or more inputs (3+ terminals total).
- Gate inputs may be declared nets or literal constants `1'b0` or `1'b1`.

### 4. Identifiers & Literals
- Identifiers: start with `[a-zA-Z_]` followed by `[a-zA-Z0-9_$]`.
- Literals: `1'b0` and `1'b1`.

### 5. Comments & Whitespace
- Single line comments: `// ...` to end of line.
- Multi-line block comments: `/* ... */`. Unclosed block comments produce an error.
- Whitespace and newlines are ignored between tokens.

---

## Unsupported Constructs (Explicitly Rejected)

The following constructs trigger an explicit compile-time error with line and column diagnostics:
- Vector/bus declarations and bit-slicing: `input [7:0] data;`
- Continuous assignments: `assign y = a & b;`
- Behavioral blocks: `always`, `initial`, `posedge`, `negedge`
- Storage variables: `reg`, `integer`, `real`, `time`
- Bidirectional ports: `inout`
- Delays: `#10`
- User-defined primitives (UDPs) and specify blocks
- Module hierarchy and named port connections: `.a(a)`
- Multiple modules in a single file

---

## Example Netlist

```verilog
// 1-bit Full Adder Netlist
module full_adder(a, b, cin, sum, cout);
  input a, b, cin;
  output sum, cout;
  wire s1, c1, c2;

  xor G1 (s1, a, b);
  xor G2 (sum, s1, cin);
  and G3 (c1, a, b);
  and G4 (c2, s1, cin);
  or  G5 (cout, c1, c2);
endmodule
```

