
# Building ngs-bits from sources (Linux/MacOS)

## Dependencies

ngs-bits depends on the following software to be installed

* _g++_ (4.5 or higher)
* _qmake_ (Qt 5.9 or higher, including xmlpatterns, charts and mysql package)
* _git_ (to extract the version hash)
* __optional:__ python and matplotlib (for plot generation in QC tools)

For example, the installation of the dependencies using Ubuntu 20.04 looks like that:

	> sudo apt-get install git make g++ qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql libqt5charts5-dev python3 python3-matplotlib libbz2-dev liblzma-dev libcurl4 libcurl4-openssl-dev zlib1g-dev
    
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

If you need to build a different version of [htslib](https://github.com/samtools/htslib), please follow [these instructions](build_htslib.md#linux_mac)

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

## Running a development server on a (local) machine

### Setting up a GSvar server for testing (on Ubuntu)

First you need a unused port on the development server. You can either select a random 5-digit port and check with `netstat -lntu` if the port is already used.
Or you can use the following 1-liner to get a free port from the kernel:

    > python -c 'import socket; s=socket.socket(); s.bind(("", 0)); print(s.getsockname()[1]); s.close()' 

Additionally you have to provide a certificate. If you do not have a valid certificate for your machine you can create a self-signed one:

    > openssl genrsa -out GSvarServer.key 2048
    > openssl req -new -key GSvarServer.key -out GSvarServer.csr
    > openssl x509 -signkey GSvarServer.key -in GSvarServer.csr -req -days 365 -out GSvarServer.crt

If you use self-signed certificates IGV and libcurl will not work.(See possible fix [below](#trust-self-signed-certificates-on-ubuntu))

Next you have to create a `GSvarServer.ini` in the `bin`folder:

    > cp bin/GSvarServer.ini.example bin/GSvarServer.ini

And fill in all required settings (like port, NGSD credentials, sample paths, ...). If the server runs on a different machine than the client you have to use the hostname/dns name of the server for `server_host`.

If you use encrypted passwords you have to add the crypt key to the `cppCORE` directory (sometimes not working on linux) or directly to the `cppCORE.pro` file. 

Next step is to build the server:

    > make build_libs_release build_server_release

And run it:

    > ./bin/GSvarServer

Now you can adapt the settings in your client and connect to the server.


### Trust self-signed certificates on Ubuntu
For the development and testing purposes it is possible to run a local instance of GSvar Server. However, if you are using self-signed certificates, you will have to make them trusted (otherwise IGV and libcurl will not be able to verify them):

Install CA certificates package:

    > sudo apt-get install ca-certificates

Copy your self-signed certificate to this location:

    > sudo cp YOUR_CERTIFICATE.crt /usr/local/share/ca-certificates

Update the list of certificate authorities:

    > sudo update-ca-certificates

