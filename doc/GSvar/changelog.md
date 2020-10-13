# GSvar change log

### 2020_06-128 (29.09.20)
- Sample search: Added sender field.
- Filters: Added filter 'somatic allele frequency'.
- General: Added support for MMSplice predictions.

### 2020_06-115 (16.09.20)
- General: Validation of CNVs/SVs can can now be requested from GSvar.
- BugFix: Fixed crash in sample similarity calculation.

### 2020_06-111 (16.09.20)
- Filters: Added loading/storing filters from/to file.
- Filters: Added option to show variants without report configuration only.
- ROHs/SVs: Added table view of OMIM column.

### 2020_06-90 (02.09.20)
- General: searching variants in the NGSD (small variants, CNVs, SVs) is now also possible from main menu (NGSD > Variants > Search...)
- General: Added dialog to search for variants published in LOVD (NGSD > Variants > Show published variants).
- General: Added copy-number histogram for regions (in menu of AF histogram).
- Report: Variant sheet was renamed to evalutation sheet and data is stored in the NGSD now.
- Report: Report configuration info now also shows the number of variants with report configuration and the number of causal variants.
- Report: Germline report configuration can now be finalized (i.e. it can no longer be changed by anyone).
- Batch import: batch import of samples can now skip samples already present in the NGSD.

### 2020_06-61 (06.08.20)
- General: Added links for variants to cBioPortal, CKB and PubMed.
- Report: Added average coverage of chrMT to germline report.
- CNVs/SVs: Added button to show target region genes overlapping with CNV/SV.
- NGSD: Added sample relation 'same patient'.

### 2020_06-40 (24.07.20)
- General: Anamnesis from GenLab can now be imported to NSGD.
- General: Added polygenic risk scores dialog (button in main tool bar) for WGS.
- General: Added repeat expansion dialog (button in main tool bar) WGS/WES.

### 2020_03-188 (24.06.20)
- CNVs: Added trio maternal contamination check for shallow WGS trios.
- CNVs: Fixed links to Decipher (chromosomes are now expected without 'chr' prefix). 
- SVs: Added search for overlapping SVs in NGSD via context menu of SVs.
- Report: Fixed language of gender field in report.

### 2020_03-159 (02.06.20)
- General: Removed step 'an' and added checkbox to to perform annotation only to analysis dialog (single-sample, multi-sample and trio analysis). Re-annotation now works for all variant calling steps (small variants, copy number variants, structural variants).
- Report: SVs can now be added to report.
- General: QC values shown in plots can now be exported as TSV.
- Somatic report: now uses germline preferred transcripts for germline variants.
- General: Added GSvar to VCF variant conversion to the main menu under `Variants`.
- General: Added cytoband to genomic coordinates conversion to the main menu under `NGSD > Regions`.
- General: sample phenotypes can now be loaded from the filter panel in the CNV/SV window
- General: added email dialog as workaround if there is no email client installed.

### 2020_03-104 (29.04.20)
- General: added Circos plot button.
- IGV: added Manta evidence BAM to IGV init dialog.
- SVs: Added NGSD annotations, NGSD-based filtering and updated default filters.

### 2020_03-63 (15.04.20)
- Gene tab: Improved matching RefSeq/CCDS transcript information (now includes UTR).
- Gene selector: now usable for WGS samples.
- General: Moved storing the GSvar file from background to the main application to avoid corrupted GSvar files.
- General: Precalcualted gap dialog now performs check for gene name and target region overlap.

### 2020_03-53 (26.03.20)
- NGSD: users can change their NGSD password ('NGSD > Admin > Change password') 
- NGSD: Admins can reset user passwords through the user table context menu.
- Gene tab: Added preferred transcript information.
- Gene tab: Added matching RefSeq/CCDS transcript information (based on cDNA sequence, ignoring UTR).
- Sample search: added causal variant information to report configuration column.
- General: AF histogram can now be shown for all variants or for filtered variants only.

### 2020_03-36 (20.03.20)
- Multi-sample analysis: fixed bug that prevented displaying variants.
- General: Added variant conversion (VCF>GSvar and HGVS.c>GSvar) via main menu > 'Variants'.
- LOVD upload: multiple HPO terms are now correctly transmitted to LOVD.


### 2020_03-11 (11.03.20)
- NGSD: added notification when there are pending validated variants.
- NSGD: added option to search for external sample name in sample search.
- General: gene database links now available through gene tab and and context menu of small variants, CNVs and SVs.
- CNV search: added size column and filter.

### 2019_11-153 (05.03.20)
- NGSD: added NGSD user name check.
- NGSD: Added transcript list to gene tab.
- NGSD: deleting processed samples is now possible (from processed sample tab and sample search tab).
- Report: Added OMIM information to XML report (only for causal variants).
- Gene selector dialog: added support for ClinCNV.

### 2019_11-129 (21.02.20)
- NGSD: added 'read %' column to sequencing run sample table.
- NGSD: added double click to open tabs (run overview, sample search, run, admin project, admin processing system)
- NGSD: added project admin page.
- NGSD: using icons to show qualities.
- NGSD: variant validation now shows transcript information (on the edit page via context menu).

### 2019_11-122 (17.02.20)
- General: moved comment, classification and variant validation from variant details to variant context menu.
- General: Added OMIM dialog (NGSD > genes > Gene OMIM info)
- Sample search: Added device filter.
- CNV search: Added copy-number filter.
- Sequencing run overview: Added column 'sample count'.

### 2019_11-115 (13.02.20)
- NGSD: Added variant validation dialog.

### 2019_11-113 (11.02.20)
- NGSD: Added batch import: MIDs, samples, processed samples.
- NGSD: Added MID clash detection.
- CNV: Added CNV search dialog (context menu of CNV dialog).

### 2019_11-99 (30.01.20)
- NGSD: Added run overview.
- NGSD: Added admin section and statistics.
- NGSD: Added edit button to NGSD tabs.
- Reports: added reports without CNV calls (chrMT).
- General: check-box for searching trio/multi analyses in "open by name" is now unchecked by default.
- Structural variants: complete re-design for dialog (contains gene names, gene overlap and OMIM column now).
- Gap report: gaps for single gene can now be calculated (if no target region is set).

### 2019_11-74 (16.01.20)
- General: moved sample search to tab.
- General: variant list is now stored in the background when changing variant classification.
- General: added check-box to "open by name" dialog to suppress searching for trio/multi analyses

### 2019_11-65 (10.01.20)
- General: added project tab (open via processed sample tab or project search).
- Project tab: moved diagnostic status overview from main menu to project tab.
- Sample tab: added sample re-analysis button.
- Sample tab: added buttons to load the sample variant list.
- Sample tab: removed NGSD button.
- Gene tab: now contains HGNC identifier (as a link to the HGNC page).
- Gene tab: now contains button to open gene variation dialog.

### 2019_11-60 (09.01.20)
- NGSD: added variant tab (allows opening variant list and processed sample tab from overview table).
- NGSD: added processing system tab (allows opening ROI in IGV or explorer).
- General: added highlighting of genes with no evidence for haploinsufficiency (cyan) in variant and CNV tables.
- Sample search: added disease group/status.

### 2019_11-40 (19.12.19)
- General: When opening a sample via NGSD, a selection dialog is shown if several analysis are available (single/trio/multi).
- Analysis status: GSvar files can now be opened via the context menu.
- Sample search: GSvar files can now be opened via the context menu.
- Sample search: Analysis can now be queued via the context menu.
- Report configuration: report configuration is now loaded automatically when a sample is opened.
- Report configuration: report configuration is now stored in NGSD every time it is modified (don't edit with two users at the same time, or you will loose data!).
- Variant details: intron/exon numbers are now correctly labeled.
- Variant details: added ClinGen link.

### 2019_11-19 (04.12.19)
- LOVD: added highlighting of RefSeq transcripts supported by LOVD.
- Disease group/status: added button to import it from GenLab.
- Variant details: extended Pfam annotation (description shown as tooltip of ID)

### 2019_11-12 (29.11.19)
- LOVD: added support for RefSeq transcripts in GSvar file.
- CNV: Fixed bug that caused QC distribution entries to disappear.
- Run tab: added setting sample quality in batch.

### 2019_09-80 (20.11.19)
- CNVs: callset quality is now automatically set if it is good.
- CNVs: added new CNV filters (genomAD o/e LOF, gene region).
- CNVs: updated default filters.
- Report: Added inheritance and classification to CNV table in report.

### 2019_09-67 (12.11.19)
- Report: show gene inheritance in addition to variant inheritance.
- Report config: set inheritance automatically, if only one option available.
- Report config: bugfix - storing should no longer crash when performing query 'INSERT INTO cnv ...'.
- SVs: removed support for Delly.
- Analysis status: added context menu entry to open last modified log file.
- Analysis status: analysis (re-)start dialogs now show quality as icon.

### 2019_09-55 (07.11.19)
- Added CNVs to report configuration (report and variant sheet). 
- CNVs: added CNV callset quality and histograms of QC metrics.
- Report: report generation now takes target region into account again.

### 2019_09-42 (16.10.19)
- Structural variants: Added target region, phenotype filter and genotype filter.

### 2019_09-29 (08.10.19)
- Report configuration: last edit date/user is now stored and shown.

### 2019_09-28 (07.10.19)
- Variant sheet: Added QR-code.

### 2019_09-18 (02.10.19)
- Report: Added report for trio analysis.

### 2019_09-17 (01.10.19)
- Report configuration: Fixed handling of preferred transcripts in variant sheet generation.
- Gene tab: Added OrphaNet data.


### 2019_09-3 (26.09.19)
- Implemented report configuration of variants (create, load/store in NGSD) and use for reports and variant sheets.
- Added 'Gene' tab.
- Gap dialog: Added category for overlap with splice regions of preferred transcript (+-6 to +-20 bases).
- Filters: added button to clear filters.
- Added highlighting of imprinting genes in CNV and ROH tables.
- Refactoring of preferred transcripts and special regions functionality.

### 2019_08-14 (12.08.19)
- Gaps: Added 'preferred transcript' column to gap recalculation dialog.
- General: Add HPO phenotypes to variant summary widget and gene summary tables.
- Minor other changes and fixes.

### 2019_08-8 (08.08.19)
- CNVs: Complete re-implementation of CNV table and filtering.
- General: Split tools menu into 'NGSD' and 'Tools'.
- Filters: Moved default filter selection to filter widget.
- Filters: Speed-up of phenotype filter by caching.

### 2019_07-27 (26.07.19)
- Removed support for ESP6500 sub-population AFs because of false-positive variants with high AF, e.g. chr2:99006157 CCCGT>C
- Subpanel design: Sub-panel template list is now searchable.

### 2019_07-8 (17.07.19)
- Variant overview dialog: fixed copy to clipboard.
- Recalulate gaps dialog: now uses Ensembl transcripts instead of CCDS.
- Processed sample tab: added sample type to sample relations table.

### 2019_05-36 (03.07.19)
- Subpanel design: is now based on Ensembl transcripts.
- Updated processed sample and sequencing run tabs.

### 2019_05-32 (02.07.19)
- Filters: region search now support single chromosomes as well.
- Processed sample tab: Added links to disease databases.
- Preferred transcripts: fixed bug that prevented preferred transcripts dialog to store edited data sometimes.

### 2019_05-11 (21.05.19)
- Processed samples: Merged samples are now modelled in NGSD and show in processed sample tab.

### 2019_05-7 (16.05.19)
- Filters: added support for genotype 'n/a' in multi-sample variant lists in (genotype affected, genotype control and trio filters)
- Bugfix: fixed crash when editing variant comment.

### 2019_04-29 (09.05.19)
- IGV: now uses the *1000Genomes build37+decoy* genome with transcript information from Ensembl and current gene names (<https://github.com/imgag/IGV_genome_ensembl>)
- Processed sample tab: Editing of sample relations is now possible.
- Somatic report: several fixes and improvements.

### 2019_04-5 (10.04.19)
- variant details: preferred transcripts are now automatically shown first.

### 2019_03-34 (25.03.19)
- Analysis status: sequencing run can now be opened via the context menu of samples.
- Processed sample tab: added buttons to open folder and load tracks in IGV.

### 2019_03-18 (13.03.19)
- General: Moved functionality for variant list from main menu to variant list tool bar.
- CNVs: Minor improvements to CNV window for ClinCNV.

### 2018_03-7 (06.03.19)
- General: Column widths remain the same when updating filters.
- General: Added support for new single-sample GSvar files that use the sample name instead of 'genotype' in the header line.
- General: Sensitive data in settings file is now encrypted.

### 2018_11-159 (04.03.19)
- General: reduced memory consumption by not showing more than 10000 variants at a time (for WGS).
- Filters: massive speed-up of trio comp-het filter (for WGS).
- IGV integration: added automatic opening of ClinCNV seg files in IGV.

### 2018_11-150 (26.02.19)
-  Switch from ExAC pLI to gnomAD o/e score

### 2018_11-146 (22.02.19)
- General: removed reference samples for IGV.
- Analysis status: moved from window into tab.
- Subpanel design is now possible with gene/transcript errors (new check box)
- CNVs: Added widget for viewing somatic and germline ClinCNV files.

### 2018_11-127 (07.02.19)
- General: Added sequencing run tabs.
- Somatic report: wording changes.
- Minor other improvements.

### 2018_11-119 (30.01.19)
- Variant details: improved visualization of large variants.
- Somatic report: Added reference values for tumor mutation burden according to DOI:10.1186/s13073-017-0424-2. 
- General: Added tab-based interface for processed sample infos.
- General: Opening processed sample tabs for arbitrary samples now possible (new icon in tool bar).
- General: Added custom column withs to main menu: `Edit -> Resize columns to content (custom)`.

### 2018_11-110 (22.01.19)
- Added NGSD variant counts of the same disease group as `NGSD_group` column.
- Gene variant info: improved impact selection.
- Analysis status dialog: speed-up of text search and added option to delete analyses.
- Report: first version of english report.

### 2018_11-100 (16.01.19)
- General: added sample relations to "processed sample" dialog.
- Variants list: added OMIM and VarSome links to variant context menu.
- Trio: added check for mendelian error rate between child/parents and warning if it is too high.

### 2018_11-79 (09.01.19)
- General: added "processed sample" dialog and plotting of quality metrics.
- Sample details: added button to show histogram of allele frequencies.

### 2018_11-55 (19.12.18)
- General: improved variant list loading and display (45% less memory, 10% faster)

### 2018_11-31 (05.12.18)
- Structural variant window: Added possibility to jump to SVs in IGV
- Sample disease info: Added support for new GenLab database schema

### 2018_11-24 (29.11.18)
- Somatic report: Bugfix for QBIC CNV report
- Somatic report: Changed numbering of CNV positions to 1-base
- Germline report: Added disease info
- Variant list: Added HGMD entry to variant context menu (search for gene).
- Gene variant info: fixed bug in 'recessive' filter

### 2018_11-7 (22.11.18)
- Somatic Report: Small changes
- Variant list: Copying non-adjacent columns to clipboard now works
- Added filter `Regulatory`
- Sample details: Added sample disease details (ICD10, HPO, CGI cancer type, tumor fraction)
	* Import from GenLab via `Sample details > NGSD >  Edit disease details`  
	* disease details are now taken from NGSD instead of GenLab (phenotype filter, somatic report, diagnostic status overview)

### 2018_10-55 (20.11.18)
- Re-analysis: button moved to main menu bar - it works for multi/trio/somatic as well now
- Germline report: added OMIM table based on panel genes
- HPO-based filter: increased flanking based used around gene loci from 100 to 5000
- Subpanel design: sub-panels can now be designed gene-wise in addition to exon-wise, added auto-generated suffix
- General: preferred transcripts can no longer be edited by two instances of GSvar in parallel

### 2018_10-43 (13.11.18)
- Report somatic: added additional somatic report for the Hochschulambulanz
- LOVD upload: fixed LOVD upload of variants on chrMT
- Variants: added highlighting of chrX, chrX, chrMT and imprinting genes
- Subpanel design: added special regions support (e.g. for POLR3A)
- CNV window: add text filter checkbox
- CNV window: add support for wildcard symbol in genes filter

### 2018_10-4 (12.10.18)
- Added support for VEP annotations (removed support for SnpEff annotations)

### 2018_06-38 (13.08.18)
- General: made 'analysis status' dialog an independent window
- Filters: added 'x-linked' and 'imprinting' to trio filter

### 2018_06-34 (07.08.2018)
- Filters: re-implemented filter concept
- Report: add analyis pipeline version to report

### 2018_06-18 (11.07.2018)
- General: Added ancestry estimation.
- Filters: The gene name filter now support using '*' as wildcard.
- ROH dialog: Child ROHs are shown for trios.
- ROH dialog: A notice is shown if UPDs are detected for trios.
- Report: Overhaul of somatic report.
- IGV: sample order from GSvar header now also defines order in IGV (somatic/trio/multi analyses)

### 2018_04-49 (15.05.2018)
- Variants: Added MitoMap/Google/SysID search to context menu.
- Variants: in-house variant counts of high-AF variants is now shown as `n/a`.

### 2018_04-37 (03.05.2018)
- Variant details: Added sample overview for a specific variant including sample similarity search.
- Sample infos: Moved sample details into right dock area.
- General: HPO terms can now be searched by synonyms as well.

### 2018_04-13 (18.04.2018)
- General: Added analysis status/queuing.
- General: Added warning for genes with "indikationsspezifische Abrechnung"
- Filters: Added context menu entry to create sub-panel from phenotype filters.

### 2018_03-8 (11.04.2018)
- Trio/Multisample: Dialog now checks that BAM files exist.
- Report: Improved somatic report.
- Structural variants: Added dialog for structural variants.

### 0.1-1039 (12.03.2018)
- Report: Improved somatic report with CGI annotations.
- General: Trio and multi-sample dialog now check that sample BAM files exist.

### 0.1-1025 (16.02.2018)
- Filters: Added gene inheritance filter based on annotations from HPO.
- Filters: removed keeping of 'anno\_high\_impact' variants from default germline filters.
- CNV dialog: Added option to use phenotype filter for variants.
- LOVD upload: fixed problem with LOVD upload of comp-het variants.
- Report: New somatic report with CGI annotations.

### 0.1-1007 (02.02.2018)
- General: Added diagnostic status overview to main menu ('Samples' > 'Diagnostic status overview').
- Improved main and gap report.

### 0.1-1001 (30.01.2018)
- General: Added more detailled diagnostic status (gene names, inheritance mode, evidence level, incidental finding).
- Filters: HPO terms can now be imported from GenLab via the contect menu of the HPO term filter.
- Report: improved formatting of variant table to make it fit to one page.

### 0.1-995 (24.01.2018)
- Filters: added phenotype-based filters (includes inheritance modes as provided by HPO).
- Filters: added filter for ExAC pLI score.
- Report: added ExAC and gnomAD allele frequency to variants in report.
- Gap dialog: the longest coding transcript is now used to determine exonic/splicing regions (to make it consistent with gaps shown in the report).
- Gap dialog: the generated text report now shows unclosed gaps as an own category.
- Variant details: fixed dbSNP links (were broken when several RS numbers were listed).

### 0.1-980 (15.01.2018)
- Fixed LOVD upload: protein change of second variant in hompound-heterozygous mode was wrong.
- Fixed gap dialog: gaps of exonic/splicing regions were sometimes too large.
- General: added warning for genes that are non-coding in GRCh37 Ensembl, but coding for GRCh38.

### 0.1-975 (12.01.2018)
- General: using IGV files instead of SEG files to visualize BAFs.
- General: now also supports ClinVar variantion IDs in addition to RCV IDs.

### 0.1-965 (03.01.2018)
- General: added GeneCards link to variant context menu.
- General: trio analysis can now be started using samples based on different processing systems.
- Gap report: now shows overall and ccds+-5 statistics separately.
- Phenotypes>Genes dialog: phenotypes can now be found by HPO id.
- ROH dialog: now shows size sum after filtering.
- Somatic: Added quick-open of somatic variant lists via tumor processed sample name.
- Sub-panels: can now be designed using existing sub-panels as template.
- LOVD upload: uploading compound-heterozygous variants together is now possible.

### 0.1-946 (15.12.2017)
- General: Multi-sample analysis can now be started when mixed processing systems were used.
- LOVD upload: report is now automatically written to transfer folder.
- Added ROH dialog button to main tool bar (next to CNV button).
- Subpanels: archive/restore dialog now allows filtering for gene(s).
- Somatic Report: Major refactoring.

### 0.1-907 (14.11.2017)
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

### 0.1-880 (11.10.2017)
- Gap dialog: added average coverage to gap report. 
- CNV dialog: added option for compound-heterozygous filtering.
- CNV dialog: added context menu entries to open CNV regions in DGV/UCSC.
- Sample information dialog: Added processed sample comment.
- Candidate gene dialog: added custom sorting via table headers. 
- Candidate gene dialog: added classification column.
- General: added default filter for recessive inheritance.

### 0.1-856 (30.08.2017)
- CNV dialog: added filter options for new CNV result columns.

### 0.1-813 (09.08.2017)
- General: gene/region and region/gene conversion now also supports non-coding genes.

### 0.1-782 (28.07.2017)
- General: Speed-up of variant list loading.
- General: Added menu entry to open sample QC data in browser (File > Open sample qcML files). 
- General: Added menu entry to open sample folder (File > Open sample folder). 
- General: Added candidate gene search dialog (Tools > Genes > Gene variant info).
- Filters: Added carrier filter.
- Report: Added KASP result to header.

### 0.1-763 (13.07.2017)
- Replaced RefSeq/UCSC transcripts by Ensembl transcripts in NGSD.

### 0.1-751 (05.07.2017)
- NGSD re-annotation: speed-up of reannotation by precalculated variant counts.

### 0.1-741 (22.06.2017)
- NGSD re-annotation: improved speed and added option to skip high-frequency variants
- General: added disease group support (sample info dialog and check if set before creating a report)

### 0.1-728 (14.06.2017)
- Filters: Added sub-population allele frequency filter.

### 0.1-722 (12.06.2017)
- General: Several preferred transcripts per gene are now possible.
- Filters: Added 'compound-het or hom' filter for affected.
- Filters: Added 'not hom' filter for controls.
- Variant details: Ensembl transcript variants now link to Ensembl website.
- Gap dialog: Added gene name filter.
- Sample information dialog: Added variant allele frequency deviation to detect sample contamination.
- Report: Added list of gene with copy-number loss/gain to somatic report.
- Report: Added list of complete/incomplete genes to CCDS coverage statistics.

### 0.1-706 (01.06.2017)
- IGV: changed genome from hg19 to GRCh37, i.e. it is now possible to jump to chrMT variants.
- Variant details: ExAC subpopulation allele frequencies are now shown when available.
- VCF export: fixed bug caused by too small buffer size.

### 0.1-693 (18.05.2017)
- Report: added CCDS+-5 bases gap statistics to report.
- CNV dialog: Added z-score based filter.
- General: Added gender determination based on SRY gene.

### 0.1-686 (04.05.2017)
- General: Added multi-sample analysis functionality.
- IGV integration: Made IGV host/port configurable through INI file.

### 0.1-666 (18.04.2017)
- General: Added NGSD annotation of target region only (for fast annotation of exome/genome samples).
- General: Added target region details dialog (in filter widget on the right).
- General: Added Alamut optional to variant context menu.
- Report: Made average depth calculation optional to allow faster creation of reports.
- Report: Added report for somatic analyses (tumor-normal pairs).
- Variant details: Fixed COSMIC links.

### 0.1-623 (06.02.2017)
- Filter: Added search functionality for target region drop-down box.
- Variant details: Removed PolyPhen2-HDIV pathogenicity prediction and added CADD/FATHMM predictions.

### 0.1-606 (09.01.2017)
- Filter: off-target variants are now called (+-50bp around target region), but not shown by default filters.
- Report: Added report dialog context menu to select/unselect all shown variants.
- General: Added option to open sub-panel design dialog after gene selector dialog.
- General: Added option to use a subpanel right after the design.

### 0.1-589 (09.12.2016)
- CNV dialog: made dialog non-modal and added button to copy filtered CNV list to clipboard.
- General: Added support to generate overview variant lists of multiple GSvar file (Tools menu).
- General: Added background export of variant/gap reports to GenLab8.

### 0.1-578 (23.11.2016)
- General: Added ExAC pLI scores for genes.
- Filter: Added option to ignore genotype for in-house database filter (used for variant list without 'genotype' column, e.g. overview variant lists with several samples).

### 0.1-568 (17.11.2016)
- General: Added first version of CNV support (button in main tool bar - see documentation for details).
- General: Added dialog to select genes based on sample-specific statistics (button in main tool bar).
- Sub-panels: Sub-panel design dialog now has an option to fall back to UCSC if no CCDS transcript is defined for a gene.

### 0.1-539 (18.10.2016)
- Filter: Replaced allele frequency filter based on ESP6500 by filter based on Kaviar database.

### 0.1-536 (13.10.2016)
- Gaps: Replaced old gap recalculation dialog with a new version for generating a gap report (main tool bar).
- Report: Removed highlighting of special variants and optional Excel-compatible variant list (no longer needed).

### 0.1-518 (26.09.2016)
- Report: Added percentage of gaps in CCDS transcripts.
- Report: Fixed truncated target file names when they contain several dots.
- Variant details view: Using local reference genome for base lookup of ExAC link now (was using a web service before, which did not always respond).

--

[back to main page](index.md)
