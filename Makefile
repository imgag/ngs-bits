# Get the Qt version from qmake
QMAKE := $(shell command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null)
QT_MAJOR := $(shell $(QMAKE) -query QT_VERSION | cut -d. -f1)

# Choose build folder based on the available Qt version
ifeq ($(QT_MAJOR),6)
	PROJECT_BUILD_DIR := ./src/build/
	PATH_TO_ROOT := ../../
	QMAKE_BIN := qmake6
else
	PROJECT_BUILD_DIR := ./
	PATH_TO_ROOT := ../src/
	QMAKE_BIN := qmake
endif

help:
	@echo "General targets:"
	@echo "  pull                  - Pulls current version from GitHub (including submodules)"
	@echo "  build_3rdparty        - Builds 3rd party libraries"
	@echo "  build_libs_release    - Builds base libraries in release mode"
	@echo "  build_tools_release   - Builds tools in release mode"
	@echo "  build_gui_release     - Builds GSvar in release mode"
	@echo "  build_server_release  - Builds GSvar server in release mode"	
	@echo "  test_lib              - Executes library tests"
	@echo "  test_tools            - Executes tool tests"
	@echo "  test_server           - Executes server tests"
	@echo "  test_release          - Builds libraries and tools in release mode and executes all tests"
	@echo "  deploy_nobuild        - Deploys current build. Note: execute 'git status' and 'make test_release' first!"
	@echo "  deploy_server_nobuild - Deploys current server build. Note: execute 'git status' and 'make test_server' first!"
	@echo "Special targets to speed up development:"
	@echo "  build_release_noclean - Build libraries and tools in release mode without cleaning up"
	@echo "  test_single_tool      - Test single tools, e.g. use 'make test_single_tool T=SeqPurge' to execute the tests for SeqPurge only"

##################################### build - DEBUG #####################################

build_info:
	@echo "**************************************************"
	@echo "*"
	@echo "* Qt major version: $(QT_MAJOR)"
	@echo "* Build dir: $(PROJECT_BUILD_DIR)"
	@echo "* Source root: $(PATH_TO_ROOT)"
	@echo "*"
	@echo "**************************************************"

build_libs_debug_noclean:
	mkdir -p $(PROJECT_BUILD_DIR)build-libs-Linux-Debug;
	cd $(PROJECT_BUILD_DIR)build-libs-Linux-Debug; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)libs.pro "CONFIG+=debug" "CONFIG-=release"; \
		make -j5;

clean_libs_debug:
	rm -rf $(PROJECT_BUILD_DIR)build-libs-Linux-Debug;

build_libs_debug: build_info clean_libs_debug build_libs_debug_noclean

build_tools_debug_noclean:
	mkdir -p $(PROJECT_BUILD_DIR)build-tools-Linux-Debug;
	cd $(PROJECT_BUILD_DIR)build-tools-Linux-Debug; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)tools.pro "CONFIG+=debug" "CONFIG-=release"; \
		make -j5;

clean_tools_debug:
	rm -rf $(PROJECT_BUILD_DIR)build-tools-Linux-Debug;

build_tools_debug: build_info clean_tools_debug build_tools_debug_noclean

build_server_debug: build_info
	rm -rf $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Debug;
	mkdir -p $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Debug;
	cd $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Debug; \
                $(QMAKE_BIN) $(PATH_TO_ROOT)tools_server.pro "CONFIG+=debug" "CONFIG-=release"; \
                make -j5;
	
#################################### build - RELEASE ####################################

build_libs_release: build_info
	rm -rf $(PROJECT_BUILD_DIR)build-libs-Linux-Release;
	mkdir -p $(PROJECT_BUILD_DIR)build-libs-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-libs-Linux-Release; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)libs.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make -j5;

build_tools_release: build_info
	rm -rf $(PROJECT_BUILD_DIR)build-tools-Linux-Release;
	mkdir -p $(PROJECT_BUILD_DIR)build-tools-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-tools-Linux-Release; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)tools.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make -j5;

build_gui_release: build_info
	rm -rf $(PROJECT_BUILD_DIR)build-tools_gui-Linux-Release;
	mkdir -p $(PROJECT_BUILD_DIR)build-tools_gui-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-tools_gui-Linux-Release; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)tools_gui.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make -j5;

build_libs_release_noclean:
	mkdir -p $(PROJECT_BUILD_DIR)build-libs-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-libs-Linux-Release; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)libs.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make -j5;

build_tools_release_noclean:
	mkdir -p $(PROJECT_BUILD_DIR)build-tools-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-tools-Linux-Release; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)tools.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make -j5;

build_gui_release_noclean:
	mkdir -p $(PROJECT_BUILD_DIR)build-tools_gui-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-tools_gui-Linux-Release; \
		$(QMAKE_BIN) $(PATH_TO_ROOT)tools_gui.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
		make -j5;

build_release_noclean: build_libs_release_noclean  build_tools_release_noclean build_gui_release_noclean

build_server_release: build_info
	rm -rf $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Release;
	mkdir -p $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Release; \
                $(QMAKE_BIN) $(PATH_TO_ROOT)tools_server.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
                make -j5;
	
build_server_release_noclean:
	mkdir -p $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Release;
	cd $(PROJECT_BUILD_DIR)build-GSvarServer-Linux-Release; \
                $(QMAKE_BIN) $(PATH_TO_ROOT)tools_server.pro "CONFIG-=debug" "CONFIG+=release" "DEFINES+=QT_NO_DEBUG_OUTPUT"; \
                make -j5;

#################################### other targets ##################################

clean:
	find src -name "*.user" | xargs rm -rf
	rm -rf $(PROJECT_BUILD_DIR)build-* $(PROJECT_BUILD_DIR)bin/out
	find $(PROJECT_BUILD_DIR)bin -type f -or -type l | grep -v ".ini" | grep -v "GSvar_" | grep -v "libhts" | xargs -l1 rm -rf

test_lib:
	cd ./bin && ./cppCORE-TEST && ./cppXML-TEST && ./cppNGS-TEST && ./cppNGSD-TEST && ./cppREST-TEST

test_lib_windows:
	cd ./bin && ./cppCORE-TEST.exe && ./cppXML-TEST.exe && ./cppNGS-TEST.exe && ./cppNGSD-TEST.exe && ./cppREST-TEST.exe

test_server:
	cd ./bin && ./GSvarServer-TEST

test_tools:
	cd ./bin && ./tools-TEST

test_tools_windows:
	cd ./bin && ./tools-TEST.exe

test_single_tool:
	cd ./bin && ./tools-TEST -s $(T)

NGSBITS_VER = $(shell  ./bin/SeqPurge --version | cut -d' ' -f2)/
DEP_PATH=/mnt/storage2/megSAP/tools/ngs-bits-$(NGSBITS_VER)
deploy_nobuild:
	@echo "#Clean up source"
	rm -rf ./bin/out ./bin/*-TEST
	@echo ""
	@echo "#Deploy binaries"
	mkdir $(DEP_PATH)
	find ./bin/ -type f  -or -type l | grep -v "settings" | xargs -I{} cp {} $(DEP_PATH)
	cp htslib/lib/libhts.* $(DEP_PATH)
	@echo ""
	@echo "#Update permissions"
	chmod 775 $(DEP_PATH)*
	@echo ""
	@echo "#Deploy settings"
	cp /mnt/storage2/megSAP/tools/ngs-bits-settings/settings_hg38.ini $(DEP_PATH)settings.ini
	@echo ""
	@echo "#Activating"
	rm /mnt/storage2/megSAP/tools/ngs-bits-current && ln -s $(DEP_PATH) /mnt/storage2/megSAP/tools/ngs-bits-current
	@echo ""
	@echo "#Settings diff:"
	diff ./bin/settings.ini $(DEP_PATH)settings.ini

	
SERVER_DEP_PATH=/opt/GSvarServer/GSvarServer-$(NGSBITS_VER)
deploy_server_nobuild:
	@if [ ! -e ./bin/GSvarServer ] ; then echo "Error: bin/GSvarServer is missing!"; false; fi;
	@if [ ! -e ./src/cppCORE/CRYPT_KEY.txt ] ; then echo "Error: src/cppCORE/CRYPT_KEY.txt is missing!"; false; fi;
	@echo "Check configuration files"
	diff /opt/GSvarServer/GSvarServer-current/GSvarServer.ini /mnt/storage2/megSAP/tools/ngs-bits-settings/GSvarServer.ini -s
	@echo "#Clean up source"
	rm -rf ./bin/out ./bin/*-TEST
	@echo ""
	@echo "#Deploy binaries"
	mkdir $(SERVER_DEP_PATH)
	find ./bin/ -type f  -or -type l | grep -v "settings" | grep -v "blat" | xargs -I{} cp {} $(SERVER_DEP_PATH)
	@echo ""
	@echo "#Create a new link"
	rm /opt/GSvarServer/GSvarServer-current && ln -s $(SERVER_DEP_PATH) /opt/GSvarServer/GSvarServer-current
	@echo ""
	@echo "#Create an empty log file"
	touch $(SERVER_DEP_PATH)/GSvarServer.log
	@echo ""
	@echo "#Deploy settings"
	cp /mnt/storage2/megSAP/tools/ngs-bits-settings/GSvarServer.ini $(SERVER_DEP_PATH)GSvarServer.ini
	@echo ""
	@echo "#Update permissions"
	chmod -R 775 $(SERVER_DEP_PATH)
	@echo ""
	@echo "#Update the user group"
	chgrp -R f_ad_bi_l_medgen_access_storages $(SERVER_DEP_PATH)

test_debug: clean build_libs_debug build_tools_debug test_lib test_tools

test_release:
	make clean build_libs_release build_tools_release build_gui_release build_server_release > t.log 2>&1
	@echo "Build done, starting tests"
	make test_lib test_tools >> t.log 2>&1
	egrep "FAILED|SKIPPED" t.log

test_release_noclean:
	make build_release_noclean test_lib test_tools > t.log 2>&1
	egrep "FAILED|SKIPPED" t.log
	
test_release_nogui:
	make clean build_libs_release build_tools_release test_lib test_tools > t.log 2>&1
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
	diff /tmp/tools.txt /tmp/tools_linked.txt | grep "<" | cut -f2 -d' ' | egrep -v "^Tsv|^NGSDImport|^NGSDExport|^NGSDAddVariants|NGSDInit|NGSDMaintain|BamCleanHaloplex"

unused_test_files:
	php doc/tools/unused_test_files.php
	
find_text:
	find src/ doc/ tools/ -name "*.md" -or -name "*.cpp" -or -name "*.h" -or -name "*.sql" -or -name "*.pro" -or -name "*.pri" | xargs -l100000 grep $(T) 

check_tool_ngsd_dependencies:
	find src/ -name "*.pro" | xargs grep lcppNGSD | cut -f2 -d/ | egrep -v "GSvar|cppNGSD-TEST" | sort > cppNGSD_should
	grep ".depends" src/tools.pro  | grep cppNGSD | cut -f1 -d'.' | egrep -v "cppNGS" | sort > cppNGSD_is
	diff cppNGSD_is cppNGSD_should
	rm -rf cppNGSD_is cppNGSD_should

dummy:

download_test_files:
	wget -O ./src/cppNGS-TEST/data_in/hg19ToHg38.over.chain.gz https://hgdownload.cse.ucsc.edu/goldenpath/hg19/liftOver/hg19ToHg38.over.chain.gz
	wget -O ./src/cppNGS-TEST/data_in/hg38ToHg19.over.chain.gz https://hgdownload.cse.ucsc.edu/goldenpath/hg38/liftOver/hg38ToHg19.over.chain.gz

#################################### 3rd party  ##################################

build_htslib:
	rm -rf htslib/build/
	mkdir -p htslib/build/
	unzip htslib/htslib_linux.zip -d ./htslib/build/
	chmod 755 htslib/build/configure
	cd htslib/build && ./configure --prefix=$(PWD)/htslib/ --enable-libcurl
	cd htslib/build && make install
	cp htslib/lib/libhts.* bin/
	rm -rf htslib/build/ htslib/bin/ htslib/share/

build_3rdparty: build_htslib
