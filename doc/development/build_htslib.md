# Building htslib for different platforms

GSvar comes with htslib's sources. It needs headers and htscodecs to work properly. The sources are stored in `ngs-bits/htslib/` folder. Whenever a new version of the library is released, the contents of the above mentioned folder have to be updated.

## <a name="linux_mac"></a>Linux (Ubuntu) and MacOS

Building process is almost identical on these platforms. Only the dependencies are installed in a slightly different way. 

 - On Ubuntu Linux run:

		> sudo apt-get install libbz2-dev liblzma-dev libcurl4 libcurl4-openssl-dev zlib1g-dev

 - On Mac (using Homebrew):

		> brew install lzlib openssl


After that follow the steps:

1. Download the latest `htslib` from [http://www.htslib.org/download/](http://www.htslib.org/download/) and unpack it.

2. Configure htslib:

    	> ./configure --enable-libcurl

3. Build htslib:

    	> make

4. Install it:

    	> make install


## <a name="windows"></a>Windows (building the DLL)

The main problem with Windows is that htslib does not maintain a build system for Windows.  

The process of building *htslib* on Windows is described in this [issue](https://github.com/samtools/htslib/issues/907).  


1. Install 64-bit `MSYS2` (which includes `mingw`) from the [project website](https://msys2.github.io/). Howerver, you should pay attention to which version you are installing, since the latest one is likely binary incompatible with the `mingw` that comes with your Qt. This `mingw` is incomplete in the sense that it does not have any build tools. Qt online installer changes over time and it is difficult to tell which version of `mingw` it currently has. You will have to manually check the version and find the corresponding `MSYS2` release. E.g. the `MSYS2` release from 2024-05-07 is compatible with `mingw` 13.1.0.  Older versions of `MSYS2` can be found [here](https://github.com/msys2/msys2-installer/releases/).

2. If behind a proxy, update proxy settings as described [here](https://stackoverflow.com/questions/29783065/msys2-pacman-cant-update-packages-through-corporate-firewall9):
	- Uncomment and set proxy credentials in `[msys]\etc\wgetrc`:
	
			https_proxy = http://[user]:[password]@[host]:[port]
			http_proxy = http://[user]:[password]@[host]:[port]
			ftp_proxy = http://[user]:[password]@[host]:[port]
	- Uncomment line in `[msys]\etc\pacman.conf`:
	
			XferCommand = /usr/bin/wget --passive-ftp -c -O %o %u


3. Open the `mingw` shell (by launching `mingw64.exe`) and install build environment (avoid using `pacman -Syu`, if you want to keep binary compatibility or install older versions of the packages):

		> pacman -S --noconfirm --needed base-devel autotools mingw-w64-x86_64-toolchain mingw-w64-x86_64-zlib mingw-w64-x86_64-bzip2 mingw-w64-x86_64-xz mingw-w64-x86_64-curl

	You may see some warnings inside the shell. Ignore them, if you did not run `pacman -Syu` and want to keep using older packages.

4. Download the latest `htslib` from [http://www.htslib.org/download/](http://www.htslib.org/download/) and unzip in some where in the [msys] directory.

5. configure htslib:
	
		> cd [htslib]
		> PATH=/mingw64/bin:$PATH
		> autoreconf -i
		> ./configure --enable-libcurl
    - `--enable-libcurl` option is needed, if you want to access files (e.g. BAM or CRAM) over HTTP/HTTPS protocol. Otherwise htslib will be able to work only with the local filesystem, accessing the server files will not be possible.
    - After executing the script you will see the results of the configuration on your screen. Make sure that the features you need have been enabled.

6. build htslib:

		> make
	
7. check htslib:

		> make check

### Deploying GSvar with htslib that supports HTTP/HTTPS

Normally, `hts-3.dll` has the following dependencies:
 * libbrotlicommon.dll
 * libbrotlidec.dll
 * libbz2-1.dll
 * libcrypto-3-x64.dll
 * libcurl-4.dll
 * libgcc_s_seh-1.dll
 * libiconv-2.dll
 * libidn2-0.dll
 * libintl-8.dll
 * liblzma-5.dll
 * libnghttp2-14.dll
 * libpsl-5.dll
 * libssh2-1.dll
 * libssl-3-x64.dll
 * libstdc++-6.dll
 * libsystre-0.dll
 * libtre-5.dll
 * libunistring-5.dll
 * libwinpthread-1.dll
 * libzstd.dll
 * zlib1.dll

However, the list may change depending on the version of the libraries. It is recommended to use the `windeployqt` utility shipped with Qt for finding libraries GSvar application depends on. Run it from a command line and specify the location of `GSvar.exe`:

        > QTDIR/bin/windeployqt GSvar.exe

The majority of GSvar dependencies will be copied to the folder with its binary executable. `windeployqt` usually handles only Qt-related libraries: use the combination of `windeployqt` and the list from above to get all the necessary dependencies. If `GSvar.exe` complains about missing DLLs, try locating them (one at a time) at the `mingw64/bin` folder and copy them next to the `GSvar.exe`. Keep adding DLLs until the application starts working. This strategy may not work in 100% of the cases (e.g. if there is a conflict between DLLs), but it significantly narrrows down the search area. More information on how to use `windeployqt` utility can be found [here](https://doc.qt.io/qt-6/windows-deployment.html)

There is a chance that reading files over HTTPS inside `GSvar` on Winodws will not work, if the system has no information about the certificate authorities needed to validate SSL certifiactes. You may need to set the `CURL_CA_BUNDLE` environment variable: it should contain the location of a CA bundle file (`ca-bundle.crt`, `ca-bundle.trust.crt`, or something else) - the file that includes root and intermediate certificates. Use `set` command for that:

        > set CURL_CA_BUNDLE=ca-bundle.crt

Now GSvar should be able to access BAM files over HTTPS and to pass the certificate validation.
