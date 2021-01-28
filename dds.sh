#!/bin/bash
for i in {1..10000}
do
  offset=$(($i+120))
  curl https://download.imgag.de/ahsturm1/genomes/GRCh37.fa -i -H "Range: bytes=$i-$offset"
done
