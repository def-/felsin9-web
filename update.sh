#!/bin/sh
set -e
mount /mnt/www-1und1 || true
cd www
jekyll build
rsync -rlptoDvP _site/ /mnt/www-1und1/nnis
fusermount -u /mnt/www-1und1 || true
