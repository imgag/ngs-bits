### 0.1-751-gf9ebc81 (05.07.2017)
- NGSD re-annotation: speed-up of reannotation by precalculated variant counts.

### 0.1-741-gb7e8a75 (22.06.2017)
- NGSD re-annotation: improved speed and added option to skip high-frequency variants
- General: added disease group support (sample info dialog and check if set before creating a report)

### 0.1-728-gb6bf123 (14.06.2017)
- Filters: Added sub-population allele frequency filter.

### 0.1-722-gaa8a403 (12.06.2017)
- General: Several preferred transcripts per gene are now possible.
- Filters: Added 'compound-het or hom' filter for affected.
- Filters: Added 'not hom' filter for controls.
- Variant details: Ensembl transcript variants now link to Ensembl website.
- Gap dialog: Added gene name filter.
- Sample information dialog: Added variant allele frequency deviation to detect sample contamination.
- Report: Added list of gene with copy-number loss/gain to somatic report.
- Report: Added list of complete/incomplete genes to CCDS coverage statistics.

### 0.1-706-g0ee82f3 (01.06.2017)
- IGV: changed genome from hg19 to GRCh37, i.e. it is now possibe to jump to chrMT variants.
- Variant details: ExAC subpopulation allele frequencies are now shown when available.
- VCF export: fixed bug caused by too small buffer size.

### 0.1-693-g3face3e (18.05.2017)
- Report: added CCDS+-5 bases gap statistics to report.
- CNV dialog: Added z-score based filter.
- General: Added gender determination based on SRY gene.

### 0.1-686-g68e17fa (04.05.2017)
- General: Added multi-sample analysis functionality.
- IGV integration: Made IGV host/port configurable through INI file.

### 0.1-666-g9924ade (18.04.2017)
- General: Added NGSD annotation of target region only (for fast annotation of exome/genome samples).
- General: Added target region details dialog (in filter widget on the right).
- General: Added Alamut optional to variant context menu.
- Report: Made averge depth calculation optional to allow faster creation of reports.
- Report: Added report for somatic analyses (tumor-normal pairs).
- Variant details: Fixed COSMIC links.

### 0.1-623-g4f16a65 (06.02.2017)
- Filter: Added search functionality for target region drop-down box.
- Variant details: Removed PolyPhen2-HDIV pathogenicity prediction and added CADD/FATHMM predictions.

### 0.1-606-g0e1e89d (09.01.2017)
- Filter: off-target variants are now called (+-50bp around target region), but not shown by default filters.
- Report: Added report dialog context menu to select/unselect all shown variants.
- General: Added option to open sub-panel design dialog after gene selector dialog.
- General: Added option to use a subpanel right after the design.

### 0.1-589-gfe0a697 (09.12.2016)
- CNV dialog: made dialog non-modal and added button to copy filtered CNV list to clipboard.
- General: Added support to generate overview variant lists of multiple GSvar file (Tools menu).
- General: Added background export of variant/gap reports to GenLab8.

### 0.1-578-g311d9bf (23.11.2016)
- General: Added ExAC pLI scores for genes.
- Filter: Added option to ignore genotype for in-house database filter (used for variant list without 'genotype' column, e.g. overview variant lists with several samples).

### 0.1-568-ga0c0b35 (17.11.2016)
- General: Added first version of CNV support (button in main tool bar - see documentation for details).
- General: Added dialog to select genes based on sample-specific statistics (button in main tool bar).
- Sub-panels: Sub-panel design dialog now has an option to fall back to UCSC if no CCDS transcript is defined for a gene.

### 0.1-539-g0977177 (18.10.2016)
- Filter: Replaced allele frequency filter based on ESP6500 by filter based on Kaviar database.

### 0.1-536-gb96fc0a (13.10.2016)
- Gaps: Replaced old gap recalculation dialog with a new version for generating a gap report (main tool bar).
- Report: Removed highlighting of special variants and optional Excel-compatible variant list (no longer needed).

### 0.1-518-geaae991 (26.09.2016)
- Report: Added percentage of gaps in CCDS transcripts.
- Report: Fixed truncated target file names when they contain several dots.
- Variant details view: Using local reference genome for base lookup of ExAC link now (was using a webservice before, which did not always respond).

--

[back to main page](index.md)







