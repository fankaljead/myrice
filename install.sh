#!bin/bash
sudo pacman-mirror -f
sudo pacman -Syy
sudo pacman -S firefox



sudo echo '[archlinuxcn]
SigLevel=Never
Server = https://mirrors.ustc.edu.cn/archlinuxcn/$arch' >> /etc/pacman.conf


sudo pacman -Syy



sudo pacman -S fcitx-im fcitx-configtool


touch ~/.xprofile
echo 'export GTK_IM_MODULE=fcitx

export QT_IM_MODULE=fcitx

export XMODIFIERS="@im=fcitx"' >> ~/.xprofile



sudo pacman -S fcitx-sogoupinyin
sudo pacman -S netease-cloud-music
sudo pacman -S yaourt



#install oh my zsh
sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"
cp ~/.oh-my-zsh/templates/zshrc.zsh-template ~/.zshrc
chsh -s /bin/zshcp ~/.oh-my-zsh/templates/zshrc.zsh-template ~/.zshrc


sudo pacman -R vim
sudo pacman -S gvim emacs
