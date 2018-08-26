" My Plugins configuration

" Plugin list
call plug#begin('~/.vim_runtime/plugged')

Plug 'junegunn/vim-easy-align'
Plug 'scrooloose/nerdtree', { 'on':  'NERDTreeToggle' }
Plug 'jiangmiao/auto-pairs'
Plug 'scrooloose/nerdcommenter'

Plug 'maralla/completor.vim'
Plug 'SirVer/ultisnips'
Plug 'honza/vim-snippets'

" Auto completion
"Plug 'tpope/vim-surround'
"Plug 'Shougo/deoplete.nvim'
"Plug 'roxma/nvim-yarp'
"Plug 'roxma/vim-hug-neovim-rpc'

" Syntax check
Plug 'w0rp/ale'
Plug 'Yggdroot/indentLine'

Plug 'sjl/gundo.vim'

call plug#end()


" Plugin Settings
"let g:deoplete#enable_at_startup = 1
let g:completor_gocode_binary = '~/go/bin/gocode'
let g:completor_clang_binary = '/usr/bin/clang'

let g:UltiSnipsExpandTrigger="<tab>"
let g:UltiSnipsJumpForwardTrigger="<c-b>"
let g:UltiSnipsJumpBackwardTrigger="<c-z>"
let g:UltiSnipsEditSplit="vertical"
