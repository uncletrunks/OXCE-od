#!/bin/sh

make -j$(nproc) || exit

convert res/linux/icons/openxcom_48x48.png  -resize 32x32! bin/icon.png

mksquashfs \
    bin/*         \
    bin/icon.png                      \
    default.gcw0.desktop oxce-$(date '+%s').opk -noappend
