FROM archlinux:latest
RUN echo 'Server = https://mirror.sjtu.edu.cn/archlinux/$repo/os/$arch' > /etc/pacman.d/mirrorlist \
    && pacman -Sy \
    && pacman -S gcc git xmake unzip zip --noconfirm \
    && cd ~ \
    && git clone https://github.com/DarkSharpness/network.git \
    && cd ~/network \
    && export XMAKE_ROOT=y \
    && xmake f -p linux -a x86_64 \
    && xmake \
    && xmake install \
    && rm -rf ~/.cache \
    && rm -rf ~/network
