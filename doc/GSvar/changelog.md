# GSvar change log

## 2025_05-16 (11.06.2025)

- added support for new megSAP pipeline with separate DRAGEN and megSAP analysis

## 2025_03-80 (20.05.25)

- RE search: added order date and patient identifier to result table
- report: float numbers in germline report are now formatted according to the selected report language
- GenLab import: fixed study import (studies were not imported if a merged sample already contained the study)
- updated CADD cutoffs according to <https://pmc.ncbi.nlm.nih.gov/articles/PMC9748256/>
- report dialog: fixed bug in report dialog (REs not included in report dialog when calling data is not in RE file)

## 2025_03-20 (02.04.25)

- improved support for DRAGEN 4.2 structural variant output

## 2025_03-13 (28.03.25)

- small variants: fixed crash in Google search from variant context menu

## 2025_03-4 (19.03.25)

- germline report: added sequencing run date

## 2025_01-41 (20.02.25)

- trio: adapted mendelian error rate calculation to Dragen 4.3 output

## 2025_01-22 (06.02.25)

- sequencing run tab: added settings dialog for NovaSeqX sample sheet creation
- variant context menu: added new PubMed search string

## 2025_01-9 (29.01.25)

- general: added 'urgent' flag to processed samples (run tab, processed sample tab, batch import)
- general: added context menu to tab bar
- small variants: added column context menu to allow faster column-specific filtering
- sample search: sample name now supports asterisk at the start/end

## 2024_11-86 (20.01.25)

- general: renamed class 'R*' to 'R' (changed in HerediCare and HerediVar)
- general: improved opening QC files from GSvarServer
- variant details: updated allOfUs variant URL
- NGSD: Added device type 'Revio'

## 2024_11-50 (07.01.25)

- general: added sample counts dialog (NGSD > Sample counts)
- variant context menu: added ProteinAtlas link for genes
- variant context menu: added UCSC Genome Browser link with ENIGMA tracks
- CNV search: added exon/splicing region overlap
- SV search: added size column for inversions

## 2024_08-102 (14.11.24)

- small variants/CNVs/SVs: added functionality for custom column order/width/visibilty (see [docu](https://github.com/imgag/ngs-bits/blob/master/doc/GSvar/configuration.md)).
- processed sample tab/sample search: added REs to report configuration summary
- IGV: speed-up of IGV init dialog by querying GSvar server in background

## 2024_08-68 (25.10.24)

- general: added HerediVar support (variant search and classification import)
- general: updated 'All of us' variant search URL
- variant classification: Added R* classification (risk allele)
- gene interpretabilty dialog: added number of coding bases
- germline report: added polamorphisms table
- processed sample tab: added user names to report config summary

## 2024_08-36 (02.10.24)

- improved RE search
- removed support for amplicon files of processing systems (not used anymore)
- fixed bugs in NSX+ sample sheet export
- somatic report: Add SV description at the start of the report

## 2024_08-18 (06.09.24)

- germline report: fixed XML validation error when generating report with RE report config for LR-sample

## 2024_08-6 (03.09.24)

- gap dialog: fixed chrMT regions wrongly shown as gaps 
- RE dialog: fixed bug preventing coloring expanded repeats when no `max. normal` is known

## 2024_07-33 (26.08.24)

- multi-sample analysis report: can now contain additional samples (configured in report dialog)
- sample search: add optional study output column
- SV search: added min/max size filter to restrict the search to variants of similar size when using `overlap` mode
- general: changed gnomAD links from v3.1.2 to v4.1
- general: added gene interpretability dialog (NGSD > genes > Gene interpretability)
- variant sheet: now contains genotype details from report configuration (de-novo, mosaic, comp-het)

## 2024_07-01 (29.07.24)

- RE dialog: fixed bug in HPO matching

## 2024_06-58 (24.07.24)

- processed sample tab: KASP user and date are now show in tooltip of KASP results
- processed sample tab: added QC metric `mosaic variant count`

## 2024_02-122 (24.06.24)

- added SVs to the somatic report configuration

## 2024_02-116 (20.06.24)

- RE dialog: Added REs to report config
- RE dialog: Implemented filtering methods
- RE dialog: Added support for lrGS
- processed sample: added flag to indicate that the sample is re-sequenced to increase depth/coverage
- sample search: Added option to search for samples with variants in NGSD only
- analysis status dialog: now shows variant caller, calling date and report config information

## 2024_02-51 (23.04.24)

- sample search: Added option to search for samples with variants in NGSD only
- analysis status: Showing variant caller information in analysis status dialog now
- report: added sequencing platform and read length to technical report
- REs: added length distribution plots

## 2024_02-38 (15.04.24)

- variant details: removed MitoMap link (no longer working from outside of the MitoMap website)
- ClinVar upload: only allowing one disease entry in ClinVar upload now
- Reworking of RE dialog (84 REs, more filter options, ...)
- QC: added chrY/chrX read ratio metric to detect chrY material in females
- general: added support for long-read trio/multi analysis

## 2024_02-6 (07.03.24)

- general: added dialog to call variants with pathogenic WT allele
- variant details: added link to `All of Us` in gnomAD allele frequency context menu
- sample search: added checkbox to add lab columns
- somatic report: several minor improvements

## 2023_11-113 (07.02.24)

- changed SpliceAI annotation (was max score, now contains gene-specific scores/positions)
- variant details dock: added Link to SpliceAI Lookup page via the SpliceAI value context menu

## 2023_11-103 (02.02.24)

- IGV: update to 2.17.0
- run tab: added checkbox to show sample comments in overview table.
- sample batch import: now highlights but skips samples already in NGSD.
- general batch import: added checking of constraints of some fields, e.g. processed sample lanes.
- general: updated LOVD search links

## 2023_11-93 (29.01.24)

- Somatic Report improvements 
- Changed "open by name" shortcut to Ctrl+Alt+O

## 2023_11-75 (15.01.24)

- IGV: no longer using 'echo' command to check if IGV is running because IGV sometimes does not reply
- report: merged CNV and SV table in technical report

## 2023_11-67 (09.01.24)

- variant details dock: best transcript is automatically shown if there is no preferred transcript
- sample tab: added gender to sample relation table
- sample relation: using hg38 coding region for sample similarity calculations now to make scores comparable between different kits

## 2023_11-53 (19.12.23)

- general: refactoring of import dialogs (runs, samples, processed samples)
- general: added functionality to open variants not contained in a sample (File > Import variants)
- IGV: improved logging to find out why IGV tracks are sometimes cleared (IGV does not respond to echo command)

## 2023_11-19 (04.12.23)

- gene tab: added search box to disease/phenotype data.
- IGV log: added command start time to IGV log window.
- bugfix: increased timeout for IGV echo command from 500 to 1500 ms to prevent unwanted clearing of IGV.
- bugfix: Fixed handling of missing region number in CNV filters.

## 2023_11-5 (21.11.23)

- IGV: improved IGV integration to make several parallel GSvar instances work with a single IGV
- variant tab: variant consequence on transcripts now shown as a table.
- sample search: added option to show variant calling information (caller, caller version, calling date)

## 2023_09-53 (15.11.23)

- open variant dialog: variant not contained in NGSD can now be added
- run tab: added CNV reference correlation to sample table
- variant tab: copying the variant to clipboard in different formats is now possible
- variant details: replaced context menu link to gnomAD 2.1 by link to gnomAD 4.0
- burden test: fixed removal of same samples (were sometimes not removed)
- burden test: added new option to define splice region
- IGV: added main menu entry to delete IGV folder
- IGV: fixed update of IGV access token (before it expired after 24 hours)

## 2023_09-53 (27.10.23)

- IGV is controlled in the background now (click IGV icon in the status bar to see details)
- replaced Sift and PolyPhen by AlphaMissense
- gene tab:  added HPO sources

## 2023_09-15 (10.10.23)

- repeat expansions: added cutoffs and general information for FGF14.
- run tab: showing molarity in sample table now
- QC plot: several instances can be opened now
- QC plot: added more filter options
- report config dialog: fixed crash when overriding small variant for indels larger than 10 bases

### 2023_06-81 (30.08.23)

- long-reads: fixed crash when filtering long-read SVs
- run tab: show columns 'processing modus' and 'batch number' in sample table
- sample search: add processed sample quality and sample quality if QC is added
- germline report: added table with gaps after closing gaps
- somatic report: allow recreation of IGV screenshot

### 2023_06-63 (18.08.23)

- update cfDNA report
- fixed PrimerDesign link
- fixed long-read report XML

### 2023_06-57 (17.08.23)

- general: added fields 'processing method' (enum) and 'batch number' (100 chars) to processed sample data and to processed sample batch import
- burden test: added exclude regions
- cfDNA report: several improvements
- GenLab import: added tissue import

### 2023_06-43 (31.07.23)

- general: fixed link to Decipher browser


### 2023_06-37 (20.07.23)

- subpanel design: previous gene names are now corrected and invalid gene names removed

### 2023_06-30 (18.07.23)

- sample/sample search/Genlab import: added year of birth 
- germline report: fixed invalid genes being shown as completely coverered
- germline report: fixed chrMT genes always being shown as gaps

### 2023_06-18 (13.07.23)

- run tab: added option to sort processed sample in order of sample entry
- general: improved variant ranking
- gereral: added burden test

### 2023_06-4 (05.07.23)

-  general: gap calculation in germline report and gap dialog is now done coding/splicing region by default and based on relevant transcripts.

### 2023_03-98 (28.06.23)

- general: Splice effect filter now supports de-novo acceptor/donor prediction of MaxEntScan as well
- RNA: improved loading times of expression dialog

### 2023_03-84 (20.06.23)

- general: enabled gap calculation for somatic single-sample analysis

### 2023_03-67 (13.06.23)

- general: updated relaxed trio filter
- somatic report: added cfDNA
- bugfix: fixed error when no filters are selected in ExpressionGeneWidget
- bugfix: fixed missing BAM files in IGV in cfDNA analysis

### 2023_03-46 (15.05.23)

- SVs: added column with genes at breakoints
- SVs: added filter for CNV overlap

### 2023_03-43 (11.05.23)

- general: added context menu entry for small variants to search for CNVs/SVs in the same gene (for compound-heterozygous variants).
- general: added small variant ranking version 2b with separate models for dominant and recessive.
- run tab: added sequencer side.
- run tab: added overall qc metrics (Q30, error rate and yield)
- bugfix: fixed crash when requesting small variant validation from tumor-only analysis.
- bugfix: fixed error when classifying a variant in trio/multi mode.
- bugfix: fixed formatting error in germline trio report.


### 2023_03-28 (25.04.23)

- somatic report: added sample type, FFPE and tissue to XML
- somatic report: updated QC in RNA report

### 2023_03-13 (30.03.23)

- report: minor improvements to germline report.
- analysis status: added checkbox to show own jobs only.

### 2023_02-53 (21.03.23)

- removed mosaic dialog (no longer needed as mosaic variant are part of the main variant list now)

### 2023_02-13 (27.02.23)

- run tab: added CNV count QC metric.
- general: added filtering for tumor and/or normal samples in QC plots.
- general: first version of cfDNA report.

### 2022_12-24 (09.01.23)

- variant tab: showing mosaic flag after genotype.
- small variant search dialog: showing mosaic flag after genotype.

### 2022_12-15 (21.12.22)

- Somatic Report: Format changes and HRD by scarHRD
- bugfixes

### 2022_11-75 (14.12.22)

- Filters: adapted filter for 'mosaic' and 'low_mappability' variants.
- Repeat expansions: updated ATXN3 cutoffs.
- Somatic: added virus table.

### 2022_11-52 (06.12.22)

- General: RNA dialogs are opened modeless now.
- Filters: added splitter handle to CNV and SV dialogs.
- Filters: filter docks windows are now collapsible.
- Filters: added target region options to filter widget: (1) copy to clip board, (2) open in IGV.
- Report configuration: variants are no longer deleted when not contained in the current variant list (e.g. when switching between trio and single sample).
- Report germline: added highlighting of relevant PRS scores.
- Report germline: now contains all relevant transcripts instead of only the best transcript.
- Report somatic: added HLA summary to XML report.
- ClinVar upload: added upload of CNVs, SVs and compound-heterozygous variants.

### 2022_11-7 (22.11.22)
- Report configuration: added manually curated HGVS type/suffix to CNV/SV.
- Report: added option to add RefSeq transcript names to HTML germline report.
- Report: moved KASP from HTML report to variant sheet.
- Sample search: added phenotype and ancestry search.
- CNV search: fixed sorting by size.
- Repeat expansions: updated cutoffs for TBP (SCA17).
- several minor bugfixes.


### 2022_10-43 (21.10.22)
- Transcripts: Added transcript versions to NGSD and using it now in report and several other places.
- Report germline: fixed long-running report generation.
- Report germline: added ClinVar upload reminder for class 4 and 5 variants.
- Report germline - XML: fixed missing classification when small variants were manually curated.

### 2022_10-18 (13.10.22)
- General: improved ordering of QC terms in processed sample tab.
- General: speed-up of low-coverage calculations (multi-threaded and pre-calculation of low-coverage regions for WGS)
- cfDNA: several improvements to panel design

### 2022_07-119 (09.09.22)
- Report: updated somatic tumor-normal report (added HLA tables for tumor and normal, formatting, fixed CNV table bug)
- Report: updated somatic RNA report (added unclear oncogenic variants table, formatting)
- Added project type filter to sequencing run overview.
- Added manual curation of variants
- Bugfixes in Expression Gene and Exon Widget

### 2022_07-92 (17.08.22)
- Report: updated somatic tumor-normal report (added target region size, formatting pathway table, added ploidy)
- Report: updated somatic RNA report (formatting)
- NGSD: added sequencing run batch import

### 2022_07-52 (03.08.22)
- Germline report: fixed bug in re-calculation of average target region depth (-1 was shown).
- Preferred transcripts dialog: added button to check entries.
- Variant tab: added ClinVar link if uploaded
- Variant tab: added information about MANE/preferred transcripts

### 2022_07-43 (01.08.22)
- General: first production version with RNA support.
- General: transcript names can now be used to open gene tabs.
- Somatic report: added pathway information.


### 2022_04-159 (01.07.22)
- Batch import: add tissue to sample batch import.
- Mosaic variants: loading phenotypes from NGSD now works in mosaic widget.
- Several minor bug fixes.

### 2022_04-127 (30.05.22)
- Gene tab: Added transcript biotype and transcript flags.
- General: Several bugfixes.

### 2022_04-109 (20.05.22)
- General: Added support for mosaic variants (called in gene exon and splice regions only).
- General: Variant tab can now be opened using different notations (GSvar, VCF, gnomAD and HGVS.c).
- Small variant filters: added phenotype settings (source, evidence level, intersection)
- Phenotype selection: added context menu entry to add parents of selected phenotype.
- Small variant details: removed 1000g AF and added gnomAD het/wt counts.
- Small variant details: RefSeq matches to Ensembl transcripts are now shown.

### 2022_04-10 (21.04.22)
- Report: added functionality to document causal varaints that are not small variants, CNVs or SVs.

### 2021_12-180 (04.04.22)
- General: Added GUI to change the permissions of restricted users.
- Report: Added gap percentage after closing gaps to germline report.

### 2021_12-162 (28.03.22)
- Gaps: gaps in closing dialog can now be opened for editing and details by double-clicking or through the context menu.

### 2021_12-138 (14.03.22)
- General: added support for somatic WES.
- NGSD: added tissue to sample table.
- Published variants: added search by gene and variant tab context menu entry.
- Small variants search: added CADD and SpliceAI to output table.
- Gaps: added context menu entry to copy coordinates.

### 2021_12-126 (03.03.22)
- General: added disease group and status to batch sample import.
- General: added batch import of sample HPO terms.

### 2021_12-112 (21.02.22)
- SVs are now contained in the XML report (germline).
- Genome coordinate lift-over is now built-in. The liftover webservice is no longer needed.

### 2021_12-100 (10.02.22)
- Added PubMed identifiers (variant details and variant tab).
- Added upload status and re-upload for ClinVar upload.
- RNA: improved integration.

### 2021_12-64 (17.01.22)
- General: Removed support for dbscSNV and MMsplice annotations.
- General: Added Google Scholar search for variants.

### 2021_12-33 (14.12.21)
- General: Added analysis information dialog to processed sample tab and analysis status tab.
- General: Improved error checks when loading a GSvar file (outdated annotation, genome build, ...).
- Sub-panels: Added functionality to manually edit a subpanel target region (sub-panel management dialog).
- General: Added BLAT search (main menu 'Tools').
- General: Genomic sequence tool (main menu 'Tools').

### 2021_09-41 (30.11.21)
- Variant details: corrected changed ClinVar variation links.
- Gene/variant tab: imiting comments to 15 lines to avoid disruption of the tab layout by huge comments.
- Somatic report: several improvements.
- General: fixed several minor issues and crashes.

### 2021_09-34 (27.10.21)
- Report: updated XML to contain HGNC gene identifiers.
- General: added lift-over dialog to main menu folder 'Conversion'.
- General: fixed several minor issues and crashes.

### 2021_09-2 (29.09.21)
- Variant table: added button to open qcML files of analysis.
- Added region to gene table conversion (Conversion entry in main menu).

### 2021_06-75 (11.08.21)
- Added ClinVar upload of variants, removed LOVD upload.
- Added option to show BAF histogram.
- The repeat expansion table can be copied to the clipboard now.

### 2021_06-57 (03.08.21)
- added cohort analysis dialog (NGSD > Variants > Cohort analysis)
- fixed DGV links (no https)

### 2021_06-38 (13.07.21)
- small variants search dialog: added maximum NGSD count filter.
- CNV search dialog: added option to scale log-likelihood, added validation column, added report config comments column.
- SV search dialog: added validation column, added report config comments column.

### 2021_06-9 (22.06.21)
- added warning dialog shown after loading a GSvar file if sample quality is 'bad', or KASP swap probability is larger than 3%.

### 2021_03-69 (20.05.21)
- General: added pre-filtering of CNVs if too many.
- Processed sample tab: added report configuration summary.

### 2021_03-66 (19.05.21)
- Processed sample tab: added ancestry score details (tool-tip).
- Sample similarity tool: Added link to documentation.
- Sample ancestry tool: Added link to documentation.

### 2021_03-54 (03.05.21)
- Somatic tumor-only report: Added summary of low coverage regions that overlap transcript.
- Somatic tumor-normal report: Added target region filter.
- Sample search: Added run start date filter to sample search.

### 2021_03-42 (22.04.21)
- Subpanels are now stored in the NGSD.

### 2021_03-34 (01.04.21)
- Report: added PRS z-scores to germline report.
- Gene tab: added imprinting information.

### 2021_03-21 (24.03.21)
- CNVs: Added mosaic CNV detection (in case a large mosaic CNV is found, a message pops up when opening the CNV window)
- Repeat expansions: Added plots (generated by the Illumina REviewer tool)'
- Processed sample tab: Added ancestry estimation for WGS samples

### 2020_12-84 (01.03.21)
- General: Genes with pseudogenes are now marked in variant tables.
- Report: Added support for generating report from multi-sample analyses.
- Report: Added option to list only one OMIM phenotype per gene in the report.
- Report: Added PRS scores to report.

### 2020_12-70 (18.02.21)
- Sample: Added import of sample relations from GenLab.

### 2020_12-40 (28.01.21)
- Gene tab: Added pseudogene information

### 2020_12-25 (15.12.20)
- General: Gaps and closed gaps are not stored in NGSD and automatically added to the report.
- General: Added support to annotate variant lists with expression data.

### 2020_12-1 (15.12.20)
- General: Added support for ENSEMBL transcript identifiers with version number.

### 2020_09-96 (11.12.20)
- IGV: IGV is now started automatically from GSvar (with a user-specific port if NGSD is enabled).
- Report: Added meta data check before a report can be created.
- Report: Variants outside the target region can now be added to the report.
- Report configuration: Inheritance is no longer set automatically (but gene inheritance is still shown).
- General: Added support for searching variants published in ClinVar.
- General: Small variant search now has an option to search for WGS/WES variants only.
- General: Variant search now has an option to search for variants of a certain project type only (small variants, CNVs, SVs).
- General: Added calculator for allele balance likelihood (Tools > Allele balance).

### 2020_09-69 (26.11.20)
- IGV: Added option to change port (see documentation).
- General: Analysis start dialogs now show disease group and status.
- General: Added support for tracking the course of tumor progression via patient-specific cfDNA panels. 

### 2020_09-44 (17.11.20)
- Ranking: Added first version of GSvar score/rank.
- Subpanels: Added file name filter to sub-panel archive/restore dialog.

### 2020_09-37 (10.11.20)
- General: removed support for GeneSplicer (replaced by MMSplice).
- Sub-panels: added file name filter to archive/restore dialog.

### 2020_09-30 (05.11.20)

- NGSD: Added disease info (group, status, HPO) and outcome to all variant search dialogs.
- NGSD: Variant classification can now be changed from the variant tab.
- NGSD: Deleting preferred transcripts is now possible.
- NGSD: Added used method to variant validation.
- NGSD: Moving processed samples between runs is now possible via context menu of the run overview.
- Studies: Added batch import for study data.
- General: Meta data tabs are no longer closed when loading a new sample.

### 2020_09-9 (08.10.20)
- Added support for studies
	- Studies can be created in NGSD > Admin > Study.
	- Processes samples can be added to studies in the processed sample tab.
	- Studies can be used as search criterion in the sample search.
- Tools: now support selecting samples from NGSD instead of the file system.

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
