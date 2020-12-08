# Checklist for a new ngs-bits release

1. Update documentation: `make build_release_noclean doc_update`
1. Update check documentation: `make doc_check_urls doc_find_missing_tools`
1. Update the tool documentation: `cd doc/toots/ && php update.php`
1. Update the changelog in `ngs-bits/README.md`.

	> git diff [last-tag] master src/cppNGSD/resources/NGSD_schema.sql  
	> git diff [last-tag] master doc/tools/
 

1. Update the download version in `ngs-bits/doc/install_*.md`.
1. Commit and push the changes.
1. Create a new release on GitHub.
1. Create a release tarball

	> cd ngs-bits/tools/releases/  
	> make create\_tarball T=[tag]  
	> make test\_tarball T=[tag] 

1. Add the tarball to the GitHub release.
1. Create a [new bioconda release](https://bioconda.github.io/contribute-a-recipe.html#update-repo) based on the release tarball.
	* Update bioconda-recipes:
			
			> git checkout master
			> git pull upstream master
			> git push origin master	
	* Create a new branch
			
			> git checkout -b ngs-bits-[tag]
	* Make changes
	* Commit and push changes
			
			> git add recipes/ngs-bits
			> git commit -m "Updated ngs-bits to version [tag]"
			> git push -u origin ngs-bits-[tag]
	* Create pull request at <https://github.com/imgag/bioconda-recipes/branches>
1. Update `megSAP/data/download_tools.sh` file and test if it works.
