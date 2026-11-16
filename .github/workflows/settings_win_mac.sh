#!/bin/bash
mkdir ./projects_folder
touch ./bin/settings.ini

# On Windows GITHUB_WORKSPACE looks like D:\a\ngs-bits\ngs-bits. Qt's INI parser
# treats backslashes in quoted values as escapes (\a, \n, ...), so convert them
# to forward slashes, which Qt accepts on Windows. On macOS this changes nothing.
WORKSPACE="${GITHUB_WORKSPACE//\\//}"

echo "reference_genome = \"$WORKSPACE/genome/GRCh38.fa\"" >> ./bin/settings.ini
echo "liftover_hg19_hg38 = ../src/cppNGS-TEST/data_in/hg19ToHg38.over.chain.gz" >> ./bin/settings.ini
echo "liftover_hg38_hg19 = ../src/cppNGS-TEST/data_in/hg38ToHg19.over.chain.gz" >> ./bin/settings.ini
