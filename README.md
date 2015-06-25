# *ngs-bits* - Short-read seqencing tools


## Obtaining ngs-bits

There are no binary releases yet.  
Please use git to download the most recent development version:

    git clone --recursive https://github.com/marc-sturm/ngs-bits.git

### Resolving proxy issues with git

If you are behind a proxy that block the standard git port, you see something like this:

    $ git clone --recursive https://github.com/marc-sturm/ngs-bits.git
    Cloning into 'ngs-bits'...
    fatal: Unable to look up github.com (port 9418) (Name or service not known)

Then you have to adapt your ~/.gitconfig file like that:

    [http]
    proxy = http://[password]@[host]:[port]


## Building ngs-bits

### Dependencies

ngs-bits depends on the following software to be installed

- g++
- qmake (Qt 5.4 or higher)
- git (to extract the version hash)
- cmake (to build bamtools library)

### Building

Just execute the following make commands:

    make build_3rdparty
	make build_tools_release

Now the executables and all required libraries can be found in the bin/ folder!

## Contributors

ngs-bits is developed and maintained by:

- Marc Sturm
- Christopher Schroeder
- Florian Lenz

## Support

Please report any issues or questions to the [ngs-bits issue 
tracker](https://github.com/marc-sturm/ngs-bits/issues).
