func! myspacevim#after() abort
  set nonu
  let s:hidden_all = 0
  set laststatus=0
  set stal=0
  set nornu
  set background = light
  colorscheme murphy
endif

function! ToggleHiddenAll()
    if s:hidden_all  == 0
        let s:hidden_all = 1
        set noshowmode
        set noruler
        set laststatus=0
        set noshowcmd
    else
        let s:hidden_all = 0
        set showmode
        set ruler
        set laststatus=2
        set showcmd
    endif
endfunction


