# ngs-bits release on GitHub

1. Update documentation: `make build_release_noclean doc_update`
1. Update check documentation: `make doc_check_urls`
1. Check for tools not added to the main page: `make doc_find_missing_tools`
1. Update the download version in `ngs-bits/README.md`.
1. Commit and push the changes.
1. Compile changelog for the new release:

	> git diff -w [last-tag] master src/cppNGSD/resources/NGSD_schema.sql  
	> git diff -w [last-tag] master doc/tools/
 
1. Create a new release on GitHub.
1. Create a release tarball

	> cd tools/releases/  
	> make create\_tarball T=[tag]  
	> make test\_tarball T=[tag] > test.log 2>&1

1. Add the tarball to the GitHub release:

	> hub release edit --draft=false --attach=ngs-bits-[tag].tgz [tag]

1. Add the Zenodo DOI of the new release to `ngs-bits/README.md`.

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
		
1. Create a pull request at <https://github.com/imgag/bioconda-recipes/branches>


# megSAP update

1. Rename and update Apptainer recipe for ngs-bits `megSAP/data/tools/container_recipes/ngs-bits_[tag].sif`
1. Build container using `php src/IMGAG/build_apptainer_container.php -tool ngs-bits -tag [tag]`.
1. Deploy the container using `php src/IMGAG/upload_apptainer_container.php -tool ngs-bits -tag [tag] -pw [password]`.
1. Update ngs-bits version in `megSAP/settings.ini.default` and `megSAP/settings_nightly.ini`.

