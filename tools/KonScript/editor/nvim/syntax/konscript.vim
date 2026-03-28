" KonScript syntax highlighting
if exists("b:current_syntax")
    finish
endif

syn keyword ksKeyword let mut const func return
syn keyword ksKeyword if else while loop for in break continue
syn keyword ksKeyword switch case default
syn keyword ksKeyword node struct enum class pub extends as spawn wait

syn keyword ksType I8 I16 I32 I64 U8 U16 U32 U64 F32 F64
syn keyword ksType Bool str String Vec2 void

syn keyword ksBuiltin InitWindow WindowShouldClose Present PollEvents
syn keyword ksBuiltin ClearBackground SetTargetFPS GetDeltaTime
syn keyword ksBuiltin DrawRectangle DrawCircle DrawLine DrawTexture DrawText
syn keyword ksBuiltin KeyDown KeyPressed KeyReleased
syn keyword ksBuiltin MouseDown MousePressed GetMouseX GetMouseY
syn keyword ksBuiltin GetNode GetChild Emit Connect
syn keyword ksBuiltin Print ToString
syn keyword ksBuiltin BeginCamera2D EndCamera2D
syn keyword ksBuiltin PlaySound PlayMusic StopSound StopMusic
syn keyword ksBuiltin LoadTexture UnloadTexture DebugMode IsDebugMode

syn keyword ksNamespace Key Mouse Gamepad

syn keyword ksBoolean true false
syn keyword ksNull    null None Some

syn region ksString start=/"/ end=/"/ skip=/\\"/ contains=ksEscape
syn match  ksEscape /\\[ntr"\\0]/ contained

syn match ksInt   /\<[0-9][0-9_]*\>/
syn match ksInt   /\<0x[0-9a-fA-F][0-9a-fA-F_]*\>/
syn match ksFloat /\<[0-9][0-9_]*\.[0-9][0-9_]*\>/

syn region ksComment start=/\/\// end=/$/ contains=ksTodo
syn region ksComment start=/\/\*/ end=/\*\// contains=ksTodo
syn keyword ksTodo contained TODO FIXME NOTE HACK XXX

syn match ksPreproc /#include/

syn match ksOperator /[-+*/%=<>!&|]/
syn match ksOperator /\.\.\=/
syn match ksOperator /->/
syn match ksOperator /=>/
syn match ksOperator /??\|?\./
syn match ksOperator /::/
syn match ksLabel    /'[a-zA-Z_][a-zA-Z0-9_]*/

syn match ksFuncDef /\<func\s\+\zs\w\+\ze\s*(/ 
syn match ksTypeDef /\<\(node\|struct\|enum\|class\)\s\+\zs\w\+/

hi def link ksKeyword   Keyword
hi def link ksType      Type
hi def link ksBuiltin   Function
hi def link ksNamespace Special
hi def link ksBoolean   Boolean
hi def link ksNull      Constant
hi def link ksString    String
hi def link ksEscape    SpecialChar
hi def link ksInt       Number
hi def link ksFloat     Float
hi def link ksComment   Comment
hi def link ksTodo      Todo
hi def link ksPreproc   PreProc
hi def link ksOperator  Operator
hi def link ksLabel     Label
hi def link ksFuncDef   Function
hi def link ksTypeDef   Type

let b:current_syntax = "konscript"
