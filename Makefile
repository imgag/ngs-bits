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

build_tools_debug:
	rm -rf build-tools-Linux-Debug;
	mkdir -p build-tools-Linux-Debug;
	cd build-tools-Linux-Debug; \
		qmake ../src/tools.pro "CONFIG+=debug" "CONFIG-=release"; \
		make;
	
#################################### build - RELEASE ####################################

build_libs_release:
	rm -rf build-libs-Linux-Release;
	mkdir -p build-libs-Linux-Release;
	cd build-libs-Linux-Release; \
		qmake ../src/libs.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make;

build_tools_release:
	rm -rf build-tools-Linux-Release;
	mkdir -p build-tools-Linux-Release;
	cd build-tools-Linux-Release; \
		qmake ../src/tools.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make;

build_gui_release:
	rm -rf build-tools_gui-Linux-Release;
	mkdir -p build-tools_gui-Linux-Release;
	cd build-tools_gui-Linux-Release; \
		qmake ../src/tools_gui.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make;

build_release_noclean:
	cd build-libs-Linux-Release; \
		qmake ../src/libs.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make;
	cd ..
	cd build-tools-Linux-Release; \
		qmake ../src/tools.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make;
	cd ..
	cd build-tools_gui-Linux-Release; \
		qmake ../src/tools_gui.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make;
	
#################################### other targets ##################################

clean:
	find src -name "*.user" | xargs rm -rf
	rm -rf build-* bin/out
	find bin -type f | grep -v ".ini" | grep -v "libhts" | xargs -l1 rm -rf

test_lib:
	cd bin && ./cppCORE-TEST && ./cppNGS-TEST && ./cppNGSD-TEST

test_tools:
	cd bin && ./tools-TEST

test_single_tool:
	cd bin && ./tools-TEST -s $(T)

DEP_PATH=/mnt/share/opt/ngs-bits-$(shell  bin/SeqPurge --version | cut -d' ' -f2)/
deploy_nobuild:
	@echo "#Clean up source"
	rm -rf bin/out bin/*-TEST
	@echo ""
	@echo "#Deploy binaries"
	mkdir $(DEP_PATH)
	find bin/ -type f  -or -type l | grep -v "settings" | xargs -I{} cp {} $(DEP_PATH)
	cp htslib/lib/libhts.* $(DEP_PATH)
	@echo ""
	@echo "#Update permissions"
	chmod 775 $(DEP_PATH)*
	@echo ""
	@echo "#Deploy settings"
	cp /mnt/share/opt/ngs-bits-settings/settings.ini $(DEP_PATH)settings.ini
	diff bin/settings.ini $(DEP_PATH)settings.ini

test_debug: clean build_libs_debug build_tools_debug test_lib test_tools

test_release:
	make clean build_libs_release build_tools_release build_gui_release test_lib test_tools > t.log 2>&1
	egrep "FAILED|SKIPPED" t.log

pull:
	git pull --recurse-submodules
	git submodule update --recursive
	git status

doc_update:
	php doc/tools/update.php
	
doc_check_urls:
	php doc/tools/check_urls.php

doc_find_missing_tools:
	ls doc/tools/ | grep .md | cut -f1 -d. | sort > /tmp/tools.txt
	grep "doc/tools/" README.md | tr "]" "[" | cut -f2 -d[ | sort > /tmp/tools_linked.txt
	diff /tmp/tools.txt /tmp/tools_linked.txt | grep "<" | cut -f2 -d' '

find_text:
	find src/ doc/ tools/ -name "*.md" -or -name "*.cpp" -or -name "*.h" -or -name "*.sql" -or -name "*.pro" -or -name "*.pri" | xargs -l100000 grep $(T) 
	
dummy:

#################################### 3rd party  ##################################

build_3rdparty: clean_3rdparty
	chmod 755 htslib/configure
	cd htslib && ./configure --prefix=$(PWD)/htslib/
	cd htslib && make install
	cd htslib && make clean
	cp htslib/lib/libhts.* bin/

clean_3rdparty:
	cd htslib && make clean
	rm -rf htslib/share htslib/lib htslib/include htslib/bin bin/libhts*
