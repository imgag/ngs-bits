help:
	cat Makefile

##################################### build - DEBUG #####################################

build_libs_debug:
	rm -rf build-libs-Linux-Debug;
	mkdir -p build-libs-Linux-Debug;
	cd build-libs-Linux-Debug; \
		qmake ../src/libs.pro "CONFIG+=debug" "CONFIG-=release"; \
		make;
	cp bamtools/lib/libbamtools.so* bin/

build_tools_debug:
	rm -rf build-tools-Linux-Debug;
	mkdir -p build-tools-Linux-Debug;
	cd build-tools-Linux-Debug; \
		qmake ../src/tools.pro "CONFIG+=debug" "CONFIG-=release"; \
		make;
	cp bamtools/lib/libbamtools.so* bin/
	
#################################### build - RELEASE ####################################

build_libs_release:
	rm -rf build-libs-Linux-Release;
	mkdir -p build-libs-Linux-Release;
	cd build-libs-Linux-Release; \
		qmake ../src/libs.pro "CONFIG-=debug" "CONFIG+=release"; \
		make;
	cp bamtools/lib/libbamtools.so* bin/

build_tools_release:
	rm -rf build-tools-Linux-Release;
	mkdir -p build-tools-Linux-Release;
	cd build-tools-Linux-Release; \
		qmake ../src/tools.pro "CONFIG-=debug" "CONFIG+=release"; \
		make;
	cp bamtools/lib/libbamtools.so* bin/

build_tools_release_noclean:
	cd build-tools-Linux-Release; \
		qmake ../src/tools.pro "CONFIG-=debug" "CONFIG+=release"; \
		make;

#################################### other targets ##################################

clean:
	find src -name "*.user" | xargs rm -rf
	rm -rf build-* bin/out
	find bin -type f | grep -v ".ini" | xargs -l1 rm -rf

test_lib:
	cd bin && ./cppCORE-TEST && ./cppNGS-TEST

test_tools:
	cd bin && ./tools-TEST

deploy_nobuild:
	rm -rf /mnt/share/opt/owntools/* bin/out bin/*-TEST 
	cp bin/* /mnt/share/opt/owntools/
	cp bamtools/lib/libbamtools.so* /mnt/share/opt/owntools/

test_debug: clean build_libs_debug build_tools_debug test_lib test_tools

test_release:
	make clean build_libs_release build_tools_release test_lib test_tools > t.log 2>&1
	egrep "FAILED|SKIPPED" t.log

dummy:

#################################### 3rd party  ##################################

build_3rdparty: clean_3rdparty
	mkdir bamtools/build
	cd bamtools/build && cmake .. && make

clean_3rdparty:
	cd bamtools && rm -rf bin build lib include src/toolkit/bamtools_version.h
