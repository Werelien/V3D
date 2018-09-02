set nocompatible

filetype off                  " required

set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()

Plugin 'VundleVim/Vundle.vim'

Plugin 'tpope/vim-fugitive'
Plugin 'tpope/vim-surround'
Plugin 'scrooloose/nerdtree'
Plugin 'scrooloose/syntastic'
Plugin 'kien/ctrlp.vim'
Plugin 'airblade/vim-gitgutter'
Plugin 'majutsushi/tagbar'
Plugin 'vim-airline/vim-airline'
Plugin 'vim-airline/vim-airline-themes'

call vundle#end()            " required

filetype plugin indent on    " required

syntax on

nnoremap <leader>ev :vsplit .vimrc<cr>
nnoremap <leader>sv :source .vimrc<cr>
nnoremap <leader>r :!./ReleaseBuildAndRun.sh<cr>
nnoremap <leader>d :!./DebugBuildAndRun.sh<cr>
nnoremap <leader>e :!./EmscriptenBuild.sh<cr>
nnoremap <leader>f :!./FDebugEmscripten.sh<cr>
set backspace=indent,eol,start
set mouse=a
set number
set relativenumber
set fillchars+=vert:\
set undofile
hi VertSplit ctermbg=darkgrey ctermfg=darkgrey

