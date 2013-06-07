#!/usr/bin/env zsh

ghci -ghci-script create.ghci

for i in *svg; do
  inkscape -z -e ${i:r}.png -d 60 -b white $i

  #gzip -9 $i
  #mv $i.gz ${i:r}.svgz

  pngnq ${i:r}.png
  optipng -o7 ${i:r}-nq8.png
  mv ${i:r}-nq8.png ${i:r}.png
done

for i in **/*.txt; do asciidoc $i; done
