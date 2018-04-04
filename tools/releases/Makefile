help:
	@cat Makefile

create_tarball:
	rm -rf ngs-bits ngs-bits-$(T).tgz
	php tarball.php -tag $(T)
	
test_tarball:
	tar xzf ngs-bits-$(T).tgz
	cd ngs-bits && make build_3rdparty
	cd ngs-bits && make build_tools_release
	ngs-bits/bin/MappingQC --help
	
