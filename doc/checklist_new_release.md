# Checklist for a new ngs-bits release

1. Update documentation: `make doc_update`
1. Update check documentation: `make doc_check_urls doc_find_missing_tools`
1. Create a new release on GitHub.
1. Create a release tarball using `ngs-bits/tools/releases/tarball.php` and add it to the GitHub release.
1. Update the download instructions in `ngs-bits/doc/install_*.md`.
1. Create a [new bioconda release](https://bioconda.github.io/contribute-a-recipe.html#update-repo) based on the release tarball.
1. Update `megSAP/data/download_tools.sh` file and test if it works.





