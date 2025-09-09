
# Building ngs-bits from sources (Linux/MacOS)

## Dependencies

ngs-bits depends on the following software to be installed

* _g++_ (4.5 or higher)
* _qmake_ (Qt 5.12 or higher, including xmlpatterns, charts and mysql package)
* _git_ (to extract the version hash)
* __optional:__ python and matplotlib (for plot generation in QC tools)

### Qt 5
Ubuntu 22.04 is the last LTS release of Ubuntu that ships with Qt 5 by default. On Ubuntu 22.04 ngs-bits requires the following packages:

        > sudo apt-get install git make g++ qtbase5-dev libqt5sql5-mysql libqt5sql5-odbc libqt5charts5-dev libqt5svg5-dev python3 python3-matplotlib libbz2-dev liblzma-dev libcurl4 libcurl4-openssl-dev zlib1g-dev pkg-config libxml2 libxml2-dev

### Qt 6
We recommend using Ubuntu 24.04, since it is the current LTS release of Ubuntu that has Qt 6 as the default Qt version. On Ubuntu 24.04 ngs-bits requires the following packages:

        > sudo apt-get install git make g++ qt6-base-dev qmake6 libqt6sql6 libqt6sql6-mysql libqt6charts6-dev libbz2-dev libqt6svg6-dev liblzma-dev zlib1g-dev libcurl4 libcurl4-openssl-dev ca-certificates libtool pkg-config libxml2 libxml2-dev
        
You will also have to create a symlink for qmake6, or adjust the `Makefile` (it uses `qmake`, not `qmake6` that comes with Qt 6):

        > sudo ln -fs /usr/bin/qmake6 /usr/bin/qmake
    
### Resolving proxy issues with git

If you are behind a proxy that blocks the standard git port, you see something like this:

    > git clone --recursive https://github.com/imgag/ngs-bits.git
    Cloning into 'ngs-bits'...
    fatal: Unable to look up github.com (port 9418) (Name or service not known)

Then you have to adapt your ~/.gitconfig file like that:

    [http]
    proxy = http://[user]:[password]@[host]:[port]


## Build

Just execute the following make commands:

    > make build_3rdparty
    > make build_libs_release
    > make build_tools_release

## Executing

Now the executables and all required libraries can be found in the `bin` folder.


### Setting the reference genome

Some of the *ngs-bits* tools need a reference genome in FASTA format.  
You can set the reference genome on the command line, e.g. the `-ref` parameter of the `VcfLeftNormalize` tool.

To avoid having to set the reference genome for each call, you can set up a settings file.  
Copy the template:

	> cp bin/settings.ini.example bin/settings.ini

and then set the `reference_genome` parameter in the `bin/settings.ini` file.  

## Setting up the NGSD (optional)

Some of the tools need the NGSD, a MySQL database that contains for example gene, transcript and exon data.  
Installation instructions for the NGSD can be found [here](install_ngsd.md).


## Building GSvar (optional)

GSvar is a GUI for viewing the variant calls produced by the [megSAP pipeline](https://github.com/imgag/megSAP).  
Additionally, it offers a  user interface to the NGSD (see above).

To build GSvar, execute the following command:

    > make build_gui_release

Now you need to [configure GSVar](GSvar/configuration.md).

GSvar is usually running in client-server mode. Thus, you need to [setup the GSvar server](GSvarServer/index.md) as well.

## Integration with IGV

For all the questions related to IGV, please see the [`IGV installation page`](GSvar\install_igv.md).
