# ngs-bits release on GitHub

1. Update documentation: `make build_release_noclean doc_update`
1. Update check documentation: `make doc_check_urls`
1. Check for tools not added to the main page: `make doc_find_missing_tools`
1. Clear the changelog and update the download versio in `ngs-bits/README.md`.
1. Commit and push the changes.
1. Compile changelog for the new release:

	> git diff -w [last-tag] master src/cppNGSD/resources/NGSD_schema.sql  
	> git diff -w [last-tag] master doc/tools/
 
1. Create a new release on GitHub.
1. Create a release tarball

	> cd tools/releases/  
	> make create\_tarball T=[tag]  
	> make test\_tarball T=[tag] 

1. Add the tarball to the GitHub release:

	> hub release edit --draft=false --attach=ngs-bits-[tag].tgz [tag]

# Bioconda release

To create a [new bioconda release](https://bioconda.github.io/contributor/workflow.html#create-a-pull-request) based on the release tarball:

1. Sync our fork and delete old branches via <https://github.com/imgag/bioconda-recipes/tree/master>
1. Create a new branch
		
		> git checkout master
		> git pull
		> git checkout -b ngs-bits-[tag]
		
1. Make changes
1. Commit and push changes
			
	> git add recipes/ngs-bits
	> git commit -m "Updated ngs-bits to version [tag]"
	> git push -u origin ngs-bits-[tag]
		
1. If the bioconda autobump bot does not create a [pull request](https://github.com/bioconda/bioconda-recipes/pulls?q=is%3Apr+ngs-bits) automatically, create a pull request at <https://github.com/imgag/bioconda-recipes/branches>


# megSAP update

1. Update and rename `megSAP/data/tools/container_recipes/ngs-bits_[tag].sif`
1. Build container using `build_ngsbits_container_release T=[tag]`.
1. Use WinSCP to copy container from `/mnt/storage2/megSAP/tools/apptainer_container/` to `megsap.de/download/container/`
1. Update ngs-bits version in `megSAP/settings.ini.default` and `megSAP/settings_nightly.ini`.

