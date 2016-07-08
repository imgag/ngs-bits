help:
	@echo "General targets:"
	@echo "  pull                  - Pulls current version from GitHub (including submodules)"
	@echo "  build_3rdparty        - Builds 3rd party libraries"
	@echo "  build_libs_release    - Builds base libraries in release mode"
	@echo "  build_tools_release   - Builds tools in release mode"
	@echo "  test_lib              - Executes library tests"
	@echo "  test_tools            - Executes tool tests"
	@echo "  test_release          - Builds libraries and tools in release mode and executes all tests"
	@echo "  deploy_nobuild        - Deploys current build. Note: execute 'git status' and 'make test_release' first!"
	@echo "Special targets to speed up development:"
	@echo "  build_release_noclean - Build libraries and tools in release mode without cleaning up"
	@echo "  test_single_tool      - Test single tools, e.g. use 'make test_single_tool T=SeqPurge' to execute the tests for SeqPurge only"

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

build_release_noclean:
	cd build-libs-Linux-Release; \
		qmake ../src/libs.pro "CONFIG-=debug" "CONFIG+=release"; \
		make;
	cd ..
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

test_single_tool:
	cd bin && ./tools-TEST -s $(T)

deploy_nobuild:
	@echo "#Clean up source"
	rm -rf bin/out bin/*-TEST 
	@echo ""
	@echo "#Clean up target"
	find /mnt/share/opt/owntools/ -type f | grep -v "settings" | xargs rm -rf
	@echo ""
	@echo "#Copy files from source to target"
	find bin/ -type f  -or -type l  | grep -v "settings" | xargs -I{} cp {} /mnt/share/opt/owntools/
	cp bamtools/lib/libbamtools.so* /mnt/share/opt/owntools/
	@echo ""
	@echo "#Diff settings"
	diff bin/settings.ini /mnt/share/opt/owntools/settings.ini
	chmod 775 /mnt/share/opt/owntools/*

test_debug: clean build_libs_debug build_tools_debug test_lib test_tools

test_release:
	make clean build_libs_release build_tools_release test_lib test_tools > t.log 2>&1
	egrep "FAILED|SKIPPED" t.log

pull:
	git pull --recurse-submodules
	git submodule update --recursive
	
dummy:

#################################### 3rd party  ##################################

build_3rdparty: clean_3rdparty
	mkdir bamtools/build
	cd bamtools/build && cmake .. && make

clean_3rdparty:
	cd bamtools && rm -rf bin build lib include src/toolkit/bamtools_version.h
