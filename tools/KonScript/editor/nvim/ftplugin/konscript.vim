" KonScript filetype plugin
" Place at: ~/.config/nvim/ftplugin/konscript.vim

if exists("b:did_ftplugin")
    finish
endif
let b:did_ftplugin = 1

" Comments
setlocal commentstring=#\ %s

" Indentation — 4 spaces
setlocal tabstop=4
setlocal shiftwidth=4
setlocal expandtab
setlocal autoindent
setlocal smartindent

" Don't re-indent lines starting with # (comments)
setlocal indentkeys-=0#

" Fold by indent
setlocal foldmethod=indent
setlocal foldlevel=99

" Matching pairs
setlocal matchpairs+=(:),{:},[:]

" Format options
setlocal formatoptions-=t   " don't auto-wrap text
setlocal formatoptions+=r   " auto-insert comment leader on <Enter>

" Word definition — include . for method chains
setlocal iskeyword+=.

" Text width
setlocal textwidth=100

" Status line hint
setlocal statusline+=[KonScript]

" ---- Indent expression ----
" Increase indent after { or (, decrease after } or )
setlocal indentexpr=KonScriptIndent()

function! KonScriptIndent()
    let prev = getline(v:lnum - 1)
    let curr = getline(v:lnum)
    let ind  = indent(v:lnum - 1)

    " Increase indent after opening brace
    if prev =~ '{\s*$'
        let ind += &shiftwidth
    endif

    " Decrease indent for closing brace
    if curr =~ '^\s*}'
        let ind -= &shiftwidth
    endif

    return ind
endfunction

" ---- Abbreviations for common patterns ----
iabbrev <buffer> nod  node  : Node2D {<CR>}<Esc>O
iabbrev <buffer> fun  func (): {<CR>}<Esc>O
iabbrev <buffer> fn   func

" ---- Mappings ----
" Ctrl+/ to toggle comment
nnoremap <buffer> <C-_> I# <Esc>
vnoremap <buffer> <C-_> :s/^\(\s*\)/\1# /<CR>

" Run the file with ksc
nnoremap <buffer> <F5> :!ksc %<CR>
nnoremap <buffer> <F6> :!ksc --compile %<CR>
