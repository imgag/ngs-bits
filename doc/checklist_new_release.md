# Checklist for a new ngs-bits release

1. Update documentation: `make build_release_noclean doc_update`
1. Update check documentation: `make doc_check_urls doc_find_missing_tools`
1. Update the changelog in `ngs-bits/README.md`.
1. Update the download version in `ngs-bits/doc/install_*.md`.
1. Commit and push the changes.
1. Create a new release on GitHub.
1. Create a release tarball using `ngs-bits/tools/releases/tarball.php` and add it to the GitHub release.
1. Create a [new bioconda release](https://bioconda.github.io/contribute-a-recipe.html#update-repo) based on the release tarball.
	* Update bioconda-recipes:
			
			> git checkout master
			> git pull upstream master
			> git push origin master	
	* Create a new branch

			> git checkout -b ngs-bits
	* Make changes
	* Commit and push changes
			
			> git add recipes/ngs-bits
			> git commit -m "Updated ngs-bits to latest version"
			> git push -u origin ngs-bits
	* Create pull request at <https://github.com/imgag/bioconda-recipes/branches>
1. Update `megSAP/data/download_tools.sh` file and test if it works.





