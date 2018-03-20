# Bioconda setup notes

## Installing bioconda

Installing bioconda was done using these steps: 

* download miniconda from `https://conda.io/miniconda.html`
* install miniconda and add it to the path
* update conda: `conda update conda`
* install conda-build: `conda install conda-build`

*Note: the steps below have already been done and miniconda3 is installed at* `/mnt/share/opt/miniconda3/bin/`

## Testing the bioconda package of ngs-bits

	> cd [path]/bioconda-recipes
	> conda-build recipes/ngs-bits/
