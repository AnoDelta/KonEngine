# known_fail/

Tests that expose real compiler bugs. These fail today and should be
fixed in the compiler before being moved back to pass/.

## 06_enums_payload.ks
**Bug**: Tagged enum (payload variant) constructor helpers reference
`EnumName_VariantName` struct that is never emitted. The tagged union
codegen in `genEnum()` is incomplete — the helper `static Shape Circle(F64 val)`
tries to return `Shape_Circle{val}` but that aggregate type is not defined.
**Fix needed in**: `codegen.hpp` → `genEnum()`

## 14_result.ks
**Bug**: `Result<I32>.Ok(value)` syntax (type-qualified static method call)
is not parseable. The parser does not support `Type<T>.Method(args)` as an
expression. There is no KonScript syntax yet for constructing a Result<T>.
**Fix needed in**: `parser.hpp` or language design decision.
