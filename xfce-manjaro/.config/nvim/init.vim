"Basic settings
set nu
set rnu
set encoding=utf-8
set autoread
set clipboard+=unnamed
syntax on
" Return to last edit position when opening files (You want this!)
au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | endif

let mapleader=" "
nmap <leader>s :w!<cr>
nmap <leader>p :bprevious<cr>
nmap <leader>n :bprevious<cr>
map <leader>' :terminal<cr>
map <leader>tn :tabnew<cr>
map <leader>to :tabonly<cr>
map <leader>tc :tabclose<cr>
map <leader>tm :tabmove

map <C-j> <C-W>j
map <C-k> <C-W>k
map <C-h> <C-W>h
map <C-l> <C-W>l


source ~/.config/nvim/plugin.vim
set laststatus=2
set t_Co=256
let g:Powerline_symbols= 'unicode'
"color dracula

