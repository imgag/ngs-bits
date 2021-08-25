FROM fedora:34
#Installing necessary utilities
RUN yum install -y \
	wget \
	wine \
	bzip2 \
	gcc \
	git

#Installing mingw packages
RUN yum install -y \ 
	mingw32-zlib.noarch \
	mingw32-zstd.noarch \
	mingw32-libzip.noarch \
	mingw32-xz.noarch \
	mingw32-xz-libs.noarch \
	mingw32-libgnurx-static.noarch \
	mingw32-gcc mingw32-binutils \
	mingw32-gcc \
	mingw32-crt \
	mingw32-headers \
	mingw32-qt5-qmake.x86_64 \
	mingw32-qt5-qtxmlpatterns.noarch \
	mingw32-qt5-qmldevtools-devel.i686 \
	mingw32-qt5-qtcharts.noarch \
	mingw32-curl-static.noarch

#Downloading htslib
RUN mkdir /root/build
WORKDIR /root/build
#RUN wget https://github.com/samtools/htslib/releases/download/1.13/htslib-1.13.tar.bz2 -P /root/build
##RUN tar -xjf /root/build/htslib-1.13.tar.bz2
##WORKDIR /root/build/htslib-1.13
#Building htslib with HTTPS support
#ENV CC=/usr/bin/i686-w64-mingw32-gcc
#RUN ./configure --host=i686-w646-mingw32 --enable-libcurl
#RUN make
ADD ./build_htslib.sh ./build_htslib.sh
ADD ./build_gsvar.sh ./build_gsvar.sh
ADD ./copy_dependencies.sh ./copy_dependencies.sh
CMD ["./build_htslib.sh", "&&", "./build_gsvar.sh"]
