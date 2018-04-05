
# Installing ngs-bits binaries (Linux/macOS)

[Bioconda](https://bioconda.github.io/) is a bioinformatics resource that extends the *Conda* package manager with bioinformatics software packages.  
*ngs-bits* binaries can be easily built using *Bioconda*.

## Setting up Bioconda (if not done yet)

To install *ngs-bits* binaries, follow these instructions:

1. Install [Miniconda](https://conda.io/miniconda.html), which installs a minimal version of the *conda* package manager.

2. Add the *Bioconda* channels using the *conda* command:
	
		> conda config --add channels defaults
		> conda config --add channels conda-forge
		> conda config --add channels bioconda

## Installing ngs-bits

Install the *ngs-bits* package using *conda*:

		> conda install ngs-bits


## Executing

Now the *ngs-bits* executables are installed into the *conda* `bin` folder.  
For example, try this:

	> bin/ReadQC --help


