# GSvar change log

### ??? (??.02.2018)
- LOVD upload: fixed problem with LOVD upload of comp-het variants.
- Filters: Added gene inheritance filter based on annotations from HPO.
- CNV dialog: Added option to use phenotype filter for variants.

### 0.1-1007-g8a14e90 (02.02.2018)
- General: Added diagnostic status overview to main menu ('Samples' > 'Diagnostic status overview').
- Improved main and gap report.

### 0.1-1001-g3e314b5 (30.01.2018)
- General: Added more detailled diagnostic status (gene names, inheritance mode, evidence level, incidental finding).
- Filters: HPO terms can now be imported from GenLab via the contect menu of the HPO term filter.
- Report: improved formatting of variant table to make it fit to one page.

### 0.1-995-g7714e35 (24.01.2018)
- Filters: added phenotype-based filters (includes inheritance modes as provided by HPO).
- Filters: added filter for ExAC pLI score.
- Report: added ExAC and gnomAD allele frequency to variants in report.
- Gap dialog: the longest coding transcript is now used to determine exonic/splicing regions (to make it consistent with gaps shown in the report).
- Gap dialog: the generated text report now shows unclosed gaps as an own category.
- Variant details: fixed dbSNP links (were broken when several RS numbers were listed).

### 0.1-980-g9addffe (15.01.2018)
- Fixed LOVD upload: protein change of second variant in hompound-heterozygous mode was wrong.
- Fixed gap dialog: gaps of exonic/splicing regions were sometimes too large.
- General: added warning for genes that are non-coding in GRCH37 Ensembl, but coding for GRCh38.

### 0.1-975-g18c7992 (12.01.2018)
- General: using IGV files instead of SEG files to visualize BAFs.
- General: now also supports ClinVar variantion IDs in addition to RCV IDs.

### 0.1-965-g86d04d8 (03.01.2018)
- General: added GeneCards link to variant context menu.
- General: trio analysis can now be started using samples based on different processing systems.
- Gap report: now shows overall and ccds+-5 statistics separately.
- Phenotypes>Genes dialog: phenotypes can now be found by HPO id.
- ROH dialog: now shows size sum after filtering.
- Somatic: Added quick-open of somatic variant lists via tumor processed sample name.
- Sub-panels: can now be designed using existing sub-panels as template.
- LOVD upload: uploading compound-heterozygous variants together is now possible.

### 0.1-946-g385628a (15.12.2017)
- General: Multi-sample analysis can now be started when mixed processing systems were used.
- LOVD upload: report is now automatically written to transfer folder.
- Added ROH dialog button to main tool bar (next to CNV button).
- Subpanels: archive/restore dialog now allows filtering for gene(s).
- Somatic Report: Major refactoring.

### 0.1-907-gc7b0851 (14.11.2017)
- General: added LOVD database link to variant context menu.
- General: LOVD upload is now also possible for variants not found by NGS.
- General: preferred transcripts can now be edited in the GUI.
- IGV: added support for b-allele frequency (BAF) files.
- Phenotype>genes dialog: genes can now be copied as comma-separated list to clipboard (for pasting into gene filter).
- Variant details: text values can now be marked my mouse and copied via the context menu.

### 0.1-890-g45f7ed29 (03.11.2017)
- General: added variant upload to LOVD from variant context menu.
- Default filters: removed AF<1% filter from somatic filter.
- Subpanel design: removed base panel setting (caused too much confusion).

### 0.1-880-ge0ae009 (11.10.2017)
- Gap dialog: added average coverage to gap report. 
- CNV dialog: added option for compound-heterozygous filtering.
- CNV dialog: added context menu entries to open CNV regions in DGV/UCSC.
- Sample information dialog: Added processed sample comment.
- Candidate gene dialog: added custom sorting via table headers. 
- Candidate gene dialog: added classification column.
- General: added default filter for recessive inheritance.

### 0.1-856-g19c01af (30.08.2017)
- CNV dialog: added filter options for new CNV result columns.

### 0.1-813-g48244c7 (09.08.2017)
- General: gene/region and region/gene conversion now also supports non-coding genes.

### 0.1-782-ge325449 (28.07.2017)
- General: Speed-up of variant list loading.
- General: Added menu entry to open sample QC data in browser (File > Open sample qcML files). 
- General: Added menu entry to open sample folder (File > Open sample folder). 
- General: Added candidate gene search dialog (Tools > Genes > Gene variant info).
- Filters: Added carrier filter.
- Report: Added KASP result to header.

### 0.1-763-g42b5a3e (13.07.2017)
- Replaced RefSeq/UCSC transcripts by Ensembl transcripts in NGSD.

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
- IGV: changed genome from hg19 to GRCh37, i.e. it is now possible to jump to chrMT variants.
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
- Report: Made average depth calculation optional to allow faster creation of reports.
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
- Variant details view: Using local reference genome for base lookup of ExAC link now (was using a web service before, which did not always respond).

--

[back to main page](index.md)















