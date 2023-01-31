# Building htslib for different platforms

GSvar comes with htslib's sources. It needs headers and htscodecs to work properly. The sources are stored in `ngs-bits/htslib/` folder. Whenever a new version of the library is released, the contents of the above mentioned folder have to be updated.

## <a name="linux_mac"></a>Linux (Ubuntu) and MacOS

Building process is almost identical on these platforms. Only the dependencies are installed in a slightly different way. 

 - On Ubuntu Linux run:

		> sudo apt-get install libbz2-dev liblzma-dev libcurl4 libcurl4-openssl-dev zlib1g-dev

 - On Mac (using Homebrew):

		> brew install lzlib openssl


After that follow the steps:

1. Download `htslib 1.16.0` from [http://www.htslib.org/download/](http://www.htslib.org/download/) and unpack it.

2. Configure htslib:

    	> ./configure --enable-libcurl

3. Build htslib:

    	> make

4. Install it:

    	> make install


## <a name="windows"></a>Windows (building the DLL)

The main problem with Windows is that htslib does not maintain a build system for Windows.  

The process of building *htslib* on Windows is described in this [issue](https://github.com/samtools/htslib/issues/907).  


1. Install 64-bit MSYS2 from the [project website](https://msys2.github.io/)

2. If behind a proxy, update proxy settings as described [here](https://stackoverflow.com/questions/29783065/msys2-pacman-cant-update-packages-through-corporate-firewall9):
	- Uncomment and set proxy credentials in `[msys]\etc\wgetrc`:
	
			https_proxy = http://[user]:[password]@[host]:[port]
			http_proxy = http://[user]:[password]@[host]:[port]
			ftp_proxy = http://[user]:[password]@[host]:[port]
	- Uncomment line in `[msys]\etc\pacman.conf`:
	
			XferCommand = /usr/bin/wget --passive-ftp -c -O %o %u


3. Open the MSYS shell **as admin** and install build environment:

		> pacman -Syu
		> pacman -S --noconfirm --needed base-devel autotools mingw-w64-x86_64-toolchain mingw-w64-x86_64-zlib mingw-w64-x86_64-bzip2 mingw-w64-x86_64-xz mingw-w64-x86_64-curl

4. Download `htslib 1.16.0` from [http://www.htslib.org/download/](http://www.htslib.org/download/) and unzip in some where in the [msys] directory.

5. configure htslib:
	
		> cd [htslib]
		> PATH=/mingw64/bin:$PATH
		> autoreconf -i
		> ./configure --enable-libcurl
    - `--enable-libcurl` option is needed, if you want to access files (e.g. BAM) over HTTP/HTTPS protocol. Otherwise htslib will be able to work only with the local filesystem, accessing the server files will not be possible.
    - After executing the script you will see the results of the configuration on your screen. Make sure that the features you need have been enabled.

6. build htslib:

		> make
	
7. check htslib:

		> make check

