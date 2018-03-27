# htslib setup notes

Current version: 1.7  
Download page: [http://www.htslib.org/download/](http://www.htslib.org/download/)

## Linux

For Linux, the the htslib shared library is built on the fly in the folder `ngs-bits/htslib` using the command:

	> make build_3rdparty


## Windows

For Windows, a pre-built DLL is present in the *ngs-bits* repository.  

### Building the DLL

The main problem with Windows is that htslib does not maintain a build system for Windows.  
The process of building *htslib 1.6* on Windows is described in this [issue](https://github.com/samtools/htslib/issues/86).  
This is the updated version for *htslib 1.7*:


1. Install 64-bit MYS2 from the [project website](https://msys2.github.io/)
2. Update proxy settings as described [here](https://stackoverflow.com/questions/29783065/msys2-pacman-cant-update-packages-through-corporate-firewall9):
	- Uncomment and set proxy credentials in `[msys1]\etc\wgetrc`:
	
			https_proxy = http://[user]:[password]@[host]:[port]
			http_proxy = http://[user]:[password]@[host]:[port]
			ftp_proxy = http://[user]:[password]@[host]:[port]
	- Uncomment line in `[msys1]\etc\pacman.conf`:
	
			XferCommand = /usr/bin/wget --passive-ftp -c -O %o %u


3. Install 64 bit build environment and configure htslib :

			> pacman -Syu
			> pacman -S base-devel
			> pacman -S mingw-w64-x86_64-toolchain
			> cd [ngs-bits]/htslib/
			> autoreconf -i
			> ./configure --disable-lzma --disable-bz2

4. Set fixed version for `PACKAGE_VERSION` and `NUMERIC_VERSION` in `Makefile`.

5. Open a CMD shell in `[ngs-bits]/htslib/` and execute

			> build_win.bat
	



