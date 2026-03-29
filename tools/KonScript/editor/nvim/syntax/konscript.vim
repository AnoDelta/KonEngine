" KonScript syntax file for Vim/Neovim
" Place at: ~/.config/nvim/syntax/konscript.vim
" (Neovim will auto-load this when filetype=konscript)

if exists("b:current_syntax")
    finish
endif

" Keywords
syntax keyword ksKeyword     let mut const func return if else while loop for in break continue switch case default node struct enum class pub as spawn wait extends
syntax keyword ksType        I8 I16 I32 I64 U8 U16 U32 U64 F32 F64 Bool str String Vec2
syntax keyword ksBuiltinType Node Node2D Sprite2D Collider2D AnimationPlayer Camera2D Sound Music Texture Scene
syntax keyword ksBoolean     true false
syntax keyword ksNull        null None Some

" Engine builtins
syntax keyword ksBuiltin
    \ InitWindow WindowShouldClose Present PollEvents ClearBackground
    \ SetTargetFPS GetDeltaTime GetFPS GetTime SetTimeScale
    \ DebugMode IsDebugMode SetVsync GetWindowWidth GetWindowHeight
    \ KeyDown KeyPressed KeyReleased
    \ MouseDown MousePressed MouseReleased
    \ GetMouseX GetMouseY GetMouseDeltaX GetMouseDeltaY GetMouseScroll
    \ LoadTexture UnloadTexture
    \ LoadSound UnloadSound PlaySound StopSound PauseSound ResumeSound IsSoundPlaying SetSoundVolume
    \ LoadMusic UnloadMusic PlayMusic StopMusic PauseMusic ResumeMusic UpdateMusic IsMusicPlaying SetMusicVolume SetMusicLooping
    \ SetMasterVolume
    \ DrawRectangle DrawCircle DrawLine DrawText DrawTexture DrawTextureRec
    \ BeginCamera2D EndCamera2D
    \ Print ToString
    \ Distance Sqrt Abs Floor Ceil Round Sin Cos Tan Atan2 Clamp Lerp Min Max

" Color presets
syntax keyword ksColor
    \ WHITE BLACK RED GREEN BLUE YELLOW CYAN MAGENTA
    \ ORANGE PURPLE GRAY DARKGRAY LIGHTGRAY PINK BROWN TRANSPARENT BLANK

" Node lifecycle methods
syntax keyword ksLifecycle Ready Update Draw OnCollisionEnter OnCollisionExit

" Strings
syntax region  ksString      start=/"/ skip=/\\./ end=/"/
syntax region  ksComment     start=/#/ end=/$/

" Numbers
syntax match   ksNumber      "\<\d\+\(\.\d*\)\?\>"
syntax match   ksNumber      "\<0x[0-9a-fA-F]\+\>"

" Operators
syntax match   ksOperator    "[+\-*/=<>!&|%^~?]"
syntax match   ksOperator    "\.\."
syntax match   ksDelimiter   "[(){}\[\],;:]"

" Member access
syntax match   ksMember      "\.\([a-zA-Z_][a-zA-Z0-9_]*\)" contains=ksMemberName
syntax match   ksMemberName  "[a-zA-Z_][a-zA-Z0-9_]*" contained

" Highlight links
highlight default link ksKeyword     Keyword
highlight default link ksType        Type
highlight default link ksBuiltinType Type
highlight default link ksBoolean     Boolean
highlight default link ksNull        Constant
highlight default link ksBuiltin     Function
highlight default link ksColor       Constant
highlight default link ksLifecycle   Special
highlight default link ksString      String
highlight default link ksComment     Comment
highlight default link ksNumber      Number
highlight default link ksOperator    Operator
highlight default link ksDelimiter   Delimiter
highlight default link ksMemberName  Identifier

let b:current_syntax = "konscript"
