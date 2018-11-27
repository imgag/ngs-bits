
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


### Adding reference genome to the settings

Some of the *ngs-bits* tools need a reference genome in FASTA format.  
You can set the reference genome on the command line, e.g. the `-ref` parameter of the `VcfLeftNormalize` tool.

To avoid having to set the reference genome for each call, you can set up a settings file.  
Copy the template:

	> cp bin/settings.ini.example bin/settings.ini

and then set the `reference_genome` parameter in the `bin/settings.ini` file.  





