#!/bin/sh
set -e
cd www
jekyll build
rsync -rlptoDvP _site/ root@ddnet:/var/www-felsing-dennis
