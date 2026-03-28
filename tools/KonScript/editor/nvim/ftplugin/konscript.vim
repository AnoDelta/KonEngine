" KonScript filetype plugin
" Sets indentation, comments, and build integration

if exists("b:did_ftplugin")
    finish
endif
let b:did_ftplugin = 1

" -----------------------------------------------------------------------
" Indentation
" -----------------------------------------------------------------------
setlocal expandtab
setlocal shiftwidth=4
setlocal tabstop=4
setlocal softtabstop=4

" -----------------------------------------------------------------------
" Comment format (for gcc/commentstring plugins)
" -----------------------------------------------------------------------
setlocal commentstring=//\ %s
setlocal comments=:///,:// 

" -----------------------------------------------------------------------
" Auto-indent on {
" -----------------------------------------------------------------------
setlocal cindent
setlocal cinkeys=0{,0},0),0],:,!^F,o,O,e
setlocal cinoptions=>s,e0,n0,f0,{0,}0,^0,:s,=s,l0,gs,hs,ps,ts,+s,c3,C0,(0,us,U0,w0,Ws,m0,M0,j0,)20,*70

" -----------------------------------------------------------------------
" Build with ksc
" -----------------------------------------------------------------------
setlocal makeprg=ksc\ %
setlocal errorformat=%f:%l:%c:\ error:\ %m

" -----------------------------------------------------------------------
" Keybindings (buffer-local)
" -----------------------------------------------------------------------
" <F5> to compile and run current file
nnoremap <buffer> <F5> :w<CR>:!ksc %<CR>

" <F6> to check (typecheck only)
nnoremap <buffer> <F6> :w<CR>:!ksc --check %<CR>

" <F7> to compile to .cpp only
nnoremap <buffer> <F7> :w<CR>:!ksc --compile %<CR>

" -----------------------------------------------------------------------
" Matching pairs
" -----------------------------------------------------------------------
setlocal matchpairs+=<:>

let b:did_ftplugin = 1
