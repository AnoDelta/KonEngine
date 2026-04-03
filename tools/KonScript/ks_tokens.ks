// ks_tokens.ks — Token kind constants, node kind constants, and shared storage
// Part of the modular KonScript self-hosted compiler

// -----------------------------------------------------------------------
// Token kind constants
// -----------------------------------------------------------------------
const TK_EOF:        I32 = 0;
const TK_INT:        I32 = 1;
const TK_FLOAT:      I32 = 2;
const TK_STR:        I32 = 3;
const TK_IDENT:      I32 = 4;
const TK_FUNC:       I32 = 5;
const TK_LET:        I32 = 6;
const TK_MUT:        I32 = 7;
const TK_RETURN:     I32 = 8;
const TK_IF:         I32 = 9;
const TK_ELSE:       I32 = 10;
const TK_WHILE:      I32 = 11;
const TK_FOR:        I32 = 12;
const TK_IN:         I32 = 13;
const TK_LOOP:       I32 = 14;
const TK_STRUCT:     I32 = 15;
const TK_ENUM:       I32 = 16;
const TK_PUB:        I32 = 17;
const TK_CONST:      I32 = 18;
const TK_AS:         I32 = 19;
const TK_BREAK:      I32 = 20;
const TK_CONTINUE:   I32 = 21;
const TK_TRUE:       I32 = 22;
const TK_FALSE:      I32 = 23;
const TK_NODE:       I32 = 24;
const TK_CLASS:      I32 = 25;
const TK_INCLUDE:    I32 = 26;
const TK_SWITCH:     I32 = 27;
const TK_SPAWN:      I32 = 28;
// New: systems programming keywords
const TK_EXTERN:     I32 = 29;
const TK_ASM:        I32 = 30;
const TK_UNION:      I32 = 31;
const TK_VOLATILE:   I32 = 32;
const TK_UNSAFE:     I32 = 33;
const TK_MOVE:       I32 = 34;

// Symbols
const TK_PLUS:       I32 = 40;
const TK_MINUS:      I32 = 41;
const TK_STAR:       I32 = 42;
const TK_SLASH:      I32 = 43;
const TK_PERCENT:    I32 = 44;
const TK_EQ:         I32 = 45;
const TK_EQEQ:       I32 = 46;
const TK_BANG:       I32 = 47;
const TK_BANGEQ:     I32 = 48;
const TK_LT:         I32 = 49;
const TK_LTEQ:       I32 = 50;
const TK_GT:         I32 = 51;
const TK_GTEQ:       I32 = 52;
const TK_AMPERAMPER: I32 = 53;
const TK_PIPEPIPE:   I32 = 54;
const TK_LPAREN:     I32 = 55;
const TK_RPAREN:     I32 = 56;
const TK_LBRACE:     I32 = 57;
const TK_RBRACE:     I32 = 58;
const TK_LBRACKET:   I32 = 59;
const TK_RBRACKET:   I32 = 60;
const TK_COMMA:      I32 = 61;
const TK_DOT:        I32 = 62;
const TK_DOTDOT:     I32 = 63;
const TK_COLON:      I32 = 64;
const TK_SEMICOLON:  I32 = 65;
const TK_ARROW:      I32 = 66;
const TK_PLUSEQ:     I32 = 67;
const TK_MINUSEQ:    I32 = 68;
const TK_STAREQ:     I32 = 69;
const TK_SLASHEQ:    I32 = 70;
const TK_PLUSPLUS:   I32 = 71;
const TK_MINUSMINUS: I32 = 72;
const TK_DOTDOTEQ:   I32 = 73;
const TK_QUESTION:   I32 = 74;
const TK_FSTR:       I32 = 75;
const TK_COLONCOLON: I32 = 76;
// New: bitwise and reference operators
const TK_AMP:        I32 = 80;  // & (single)
const TK_PIPE:       I32 = 81;  // | (single)
const TK_CARET:      I32 = 82;  // ^
const TK_TILDE:      I32 = 83;  // ~
const TK_LTLT:       I32 = 84;  // <<
const TK_GTGT:       I32 = 85;  // >>
const TK_AMPEQ:      I32 = 86;  // &=
const TK_PIPEEQ:     I32 = 87;  // |=
const TK_CARETEQ:    I32 = 88;  // ^=

// -----------------------------------------------------------------------
// Node kind constants
// -----------------------------------------------------------------------
const NK_NULL:      I32 = 0;
const NK_INT:       I32 = 1;
const NK_FLOAT:     I32 = 2;
const NK_BOOL:      I32 = 3;
const NK_STR_LIT:   I32 = 4;
const NK_IDENT:     I32 = 5;
const NK_BINARY:    I32 = 6;
const NK_UNARY:     I32 = 7;
const NK_CALL:      I32 = 8;
const NK_MEMBER:    I32 = 9;
const NK_INDEX:     I32 = 10;
const NK_ASSIGN:    I32 = 11;
const NK_LIST:      I32 = 12;
const NK_LET:       I32 = 13;
const NK_CONST_D:   I32 = 14;
const NK_RETURN:    I32 = 15;
const NK_IF:        I32 = 16;
const NK_WHILE:     I32 = 17;
const NK_FOR_IN:    I32 = 18;
const NK_LOOP:      I32 = 19;
const NK_BREAK:     I32 = 20;
const NK_CONTINUE:  I32 = 21;
const NK_BLOCK:     I32 = 22;
const NK_FUNC:      I32 = 23;
const NK_PARAM:     I32 = 24;
const NK_PROGRAM:   I32 = 25;
const NK_CAST:      I32 = 26;
const NK_ARRAY_LIT: I32 = 27;
const NK_INCLUDE_D: I32 = 28;
const NK_NULL_LIT:  I32 = 29;
const NK_STRUCT_D:  I32 = 30;
const NK_FIELD:     I32 = 31;
// New: systems programming and safety nodes
const NK_EXTERN:    I32 = 32;  // extern "C" func decl; str="name|linkage|rettype"
const NK_ASM:       I32 = 33;  // asm("template" : out : in : clobber)
const NK_UNION_D:   I32 = 34;  // union type declaration
const NK_REF:       I32 = 35;  // &expr (shared reference)
const NK_REF_MUT:   I32 = 36;  // &mut expr (mutable reference)
const NK_DEREF:     I32 = 37;  // *expr (dereference)
const NK_FUNC_EXPR: I32 = 38;  // closure: func(params) -> T { body }
const NK_FSTR:      I32 = 40;  // f-string

// -----------------------------------------------------------------------
// Token storage — parallel arrays, one slot per token
// -----------------------------------------------------------------------
let mut tok_kinds:  [I32] = [0];
let mut tok_values: [Str] = [""];
let mut tok_lines:  [I32] = [0];
let mut tok_cols:   [I32] = [0];

// IR emit counters
let mut tmp_count: I32 = 0;
let mut str_count: I32 = 0;

// ── AST storage — parallel arrays, index 0 is the null node ─────────────
let mut node_kinds: [I32] = [0];
let mut node_a:     [I32] = [0];
let mut node_b:     [I32] = [0];
let mut node_c:     [I32] = [0];
let mut node_str:   [Str] = [""];
let mut node_line:  [I32] = [0];
