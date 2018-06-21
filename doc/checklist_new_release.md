# Checklist for a new ngs-bits release

1. Create a new release on GitHub.
3. Create a release tarball using `ngs-bits/tools/releases/tarball.php` and add it to the GitHub release.
2. Update the download instructions in `ngs-bits/doc/install_*.md`.
4. Create a [new bioconda release](https://bioconda.github.io/contribute-a-recipe.html#update-repo) based on the release tarball.
5. Update `megSAP/data/download_tools.sh` file and test if it works.




