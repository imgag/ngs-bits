## RNA analysis

RNAseq data analysis generates the following information:

- genomic alignment
- gene-level and exon-level expression values
- relative expression values, compared to cohort (gene and exon)
- annotated splicing junctions, detection of non-reference events
- fusion transcripts

For DNA samples linked with RNA, DNA variants are enhanced with the
following information:

- frequency and coverage of variant in RNA
- probability for allele-specific expression (for HET variants)

In addition, expression values can be explored across cohorts and
processing systems.

All RNAseq information is available from the corresponding DNA
sample. DNA-RNA sample relation is required and usually set
automatically when processing sequencing runs which include RNA
samples. Re-annotation is required after analysis of RNA sample, it
is usually queued automatically.

![GSvar processed sample view, highlight sample relation](rna_sample_relation.png)

### Opening RNA tracks

RNA tracks can be selected in the "Initialize IGV" window.

![initialize IGV window, highlight RX/fusions/splicing](rna_igv_menu.png)

### Enhanced variant information

Variants are annotated with information from RNA sample.

The following columns are added to the GSvar file and can be seen in the variant view:

- ASE_af: Expressed variant allele frequency
- ASE_depth: Sequencing depth at the variant position
- ASE_alt: Expressed variant alternative observation count
- ASE_pval: Binomial test p-value
- aberrant_splicing: aberrant splicing fraction of annotated genes

**TODO** list new columns 

A summary of the RNA values can also be seen in the bottom right tile of GSvar:
![rna variant tile](rna_variant_info.png)

There are also filter for these columns:
![rna variant filter in GSvar](rna_variant_filter.png)

### Gene expression

Sample expression data can be viewed with *RNA*, *Show expression data*.

*Cohort determination* defines which samples are used as background
data, this is only available for gene-level expression. Exon-level
expression is pre-computed and uses the most relevant cohort
strategy.

The following filters can be used to find relevant genes:

- *Gene*: list of gene symbols, comma-separated
- *min. abs. logFC*: minimum absolute log2 fold-change, expression
  change
- *min. abs. z-score*: minimum absolute z-score, outlier status
- *min. TPM (sample)*: minimum TPM (transcripts per million), low
  expression cut-off in sample
- *min. TPM (cohort)*: minimum mean TPM in cohort, low
  expression cut-off in cohort
- *low expression (TPM)*: minimum TPM in cohort or sample, to
  filter genes with generally low expression
- *biotype*: gene biotype (only *protein coding* preselected)

### Splicing

Splicing junctions detected and quantified during read alignment
are visualized as an additional track in IGV. Splicing events are
annotated, the following event types are used to describe an
observed junction:

- *known*: matches known transcript
- *exon-truncation*: upstream/downstream exon is truncated
  (splicing junction inside exon)
- *exon-skip*: at least one exon skipped
- *exon-skip-truncation*: combination of exon truncation and
  skipping
- *intronic-or-overlap-no-tx*: no supporting transcript, junctions
  inside intronic region (at least one end), hints to novel exon
- *other*: other unspecific events, e.g. intergenic region

The number of reads supporting the observed junction and the
splicing motif is shown by hovering or clicking the event.

![IGV with splicing track scaled to one gene](rna_igv_splicing.png)

### Fusions

Potential fusion transcripts are listed in *RNA*, *Show RNA
fusions*.

**TODO** PDF file with visual report

### Quality Control

The following QC values are checked after analysis:

| | PAXgene blood | fibroblast culture |
|---|---|---|
| sequencing depth | 50M cluster/100M reads | 25M cluster/50M reads |
| housekeeping genes coverage % | >65% (20x), or >80% (10x) | >80% (20x), or >90% (10x) |
| covered genes | >13.000 | >13.000 |
| on target (coding exons) % | >40% | >60% |

--

[back to main page](index.md)
