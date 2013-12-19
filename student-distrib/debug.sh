#!/bin/sh
set -x
if [ -d /mnt/tmpmp3 ]; then
umount -l /mnt/tmpmp3
rmdir /mnt/tmpmp3
fi

if [ -d /tmp/mp3 ]; then
rm -rf /tmp/mp3
fi

mkdir /mnt/tmpmp3
mkdir /tmp/mp3
cp ./bootimg /tmp/mp3/
cp ./filesys_img /tmp/mp3/
cp ./mp3.img /tmp/mp3/
mount -o loop,offset=32256 /tmp/mp3/mp3.img /mnt/tmpmp3
cp -f /tmp/mp3/bootimg /mnt/tmpmp3/
cp -f /tmp/mp3/filesys_img /mnt/tmpmp3/
umount -l /mnt/tmpmp3
cp -f /tmp/mp3/mp3.img ./
rm -rf /tmp/mp3
rmdir /mnt/tmpmp3

