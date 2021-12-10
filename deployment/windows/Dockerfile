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
	mingw32-curl-static.noarch \	
	mingw32-qt5-qtbase-devel.i686

#Building binaries
RUN mkdir /root/build
RUN mkdir /root/output
WORKDIR /root/build
ADD ./make_build.sh ./make_build.sh

CMD ["./make_build.sh"]
