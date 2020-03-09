# *ngs-bits* - Short-read sequencing tools

[![TravisCI status](https://api.travis-ci.org/imgag/ngs-bits.svg?branch=master)](https://travis-ci.org/imgag/ngs-bits/builds)
[![install with bioconda](https://img.shields.io/badge/install%20with-bioconda-brightgreen.svg?style=flat-square)](http://bioconda.github.io/recipes/ngs-bits/README.html)

## Obtaining ngs-bits

Binaries of *ngs-bits* are available via Bioconda. Alternatively, *ngs-bits* can be built from sources:

* **Binaries** for [Linux/macOS](doc/install_bioconda.md)
* From **sources** for [Linux/macOS](doc/install_unix.md)
* From **sources** for [Windows](doc/install_win.md)

## ChangeLog

Changes already implemented in GIT master for next release:

* none so far

Changes in release 2020_03:

* Added tools: BedpeGeneAnnotation, BedpeAnnotateFromBed, NGSDAnnotateCNV
* NGSDImportORPHA: Added handling of outdated gene names.
* NGSDExportSamples: Added report config output and a filter for analyzed runs only.
* VcfCheck: added test for invalid characters in INFO column, added support for VCF.GZ files.
* NGSDExportGenes: made HPO-annotation optional because it is slow.



For older releases see the [releases page](https://github.com/imgag/ngs-bits/releases).

## Support

Please report any issues or questions to the [ngs-bits issue 
tracker](https://github.com/imgag/ngs-bits/issues).

## Documentation

Have a look at the [ECCB'2018 poster](doc/data/poster_ECCB2018.pdf).

The documentation of individual tools is linked in the tools list below.  
For some tools the documentation pages contain only the command-line help, for other tools they contain more information.

## License

*ngs-bits* is provided under the [MIT license](LICENSE) and is based on other open source software:

* [htslib](https://github.com/samtools/htslib) for HTS data format support (BAM, VCF, ...)
* [SimpleCrypt](https://wiki.qt.io/Simple_encryption_with_SimpleCrypt) for weak encryption
* [QR-Code-generator](https://github.com/nayuki/QR-Code-generator) for QR code generation


## Tools list

_ngs-bits_ contains a lot of tools that are used for NGS-based diagnostics in our [institute](http://www.uni-tuebingen.de/Klinische_Genetik/start.html).

Some of the tools need the NGSD, a database that contains for example gene, transcript and exon data.  
Installation instructions for the NGSD can be found [here](doc/install_ngsd.md).

### Main tools

* [SeqPurge](doc/tools/SeqPurge/index.md) - A highly-sensitive adapter trimmer for paired-end short-read data.
* [SampleSimilarity](doc/tools/SampleSimilarity/index.md) - Calculates pairwise sample similarity metrics from VCF/BAM files.
* [SampleGender](doc/tools/SampleGender.md) - Determines sample gender based on a BAM file.
* [SampleAncestry](doc/tools/SampleAncestry/index.md) - Estimates the ancestry of a sample based on variants.
* [CnvHunter](doc/tools/CnvHunter/index.md) - CNV detection from targeted resequencing data using non-matched control samples.
* [RohHunter](doc/tools/RohHunter/index.md) - ROH detection based on a variant list annotated with AF values.
* [UpdHunter](doc/tools/UpdHunter.md) - UPD detection from trio variant data.

### QC tools

The default output format of the quality control tools is [qcML](https://github.com/HUPO-PSI/qcML-development/), an XML-based format for -omics quality control, that consists of an [XML schema](https://github.com/HUPO-PSI/qcML-development/blob/master/schema/v0_0_8/qcML_0.0.8.xsd), which defined the overall structure of the format, and an [ontology](https://github.com/HUPO-PSI/qcML-development/blob/master/cv/qc-cv.obo) which defines the QC metrics that can be used.

* [ReadQC](doc/tools/ReadQC.md) - Quality control tool for FASTQ files.
* [MappingQC](doc/tools/MappingQC.md) - Quality control tool for a BAM file.
* [VariantQC](doc/tools/VariantQC.md) - Quality control tool for a VCF file.
* [SomaticQC](doc/tools/SomaticQC.md) - Quality control tool for tumor-normal pairs ([paper](https://www.ncbi.nlm.nih.gov/pubmed/28130233) and [example output data](doc/data/somatic_qc.zip?raw=true)).

### BAM tools

* [BamClipOverlap](doc/tools/BamClipOverlap.md) - (Soft-)Clips paired-end reads that overlap.
* [BamDownsample](doc/tools/BamDownsample.md) - Downsamples a BAM file to the given percentage of reads.
* [BamFilter](doc/tools/BamFilter.md) - Filters a BAM file by multiple criteria.
* [BamHighCoverage](doc/tools/BamHighCoverage.md) - Determines high-coverage regions in a BAM file.
* [BamToFastq](doc/tools/BamToFastq.md) - Converts a BAM file to FASTQ files (paired-end only).

### BED tools

* [BedAdd](doc/tools/BedAdd.md) - Merges regions from several BED files.
* [BedAnnotateFromBed](doc/tools/BedAnnotateFromBed.md) - Annotates BED file regions with information from a second BED file.
* [BedAnnotateGC](doc/tools/BedAnnotateGC.md) - Annnotates the regions in a BED file with GC content.
* [BedAnnotateGenes](doc/tools/BedAnnotateGenes.md) - Annotates BED file regions with gene names (needs [NGSD](doc/install_ngsd.md)).
* [BedChunk](doc/tools/BedChunk.md) - Splits regions in a BED file to chunks of a desired size.
* [BedCoverage](doc/tools/BedCoverage.md) - Annotates the regions in a BED file with the average coverage in one or several BAM files.
* [BedExtend](doc/tools/BedExtend.md) - Extends the regions in a BED file by _n_ bases.
* [BedGeneOverlap](doc/tools/BedGeneOverlap.md) - Calculates how much of each overlapping gene is covered (needs [NGSD](doc/install_ngsd.md)).
* [BedInfo](doc/tools/BedInfo.md) - Prints summary information about a BED file.
* [BedIntersect](doc/tools/BedIntersect.md) - Intersects two BED files.
* [BedLowCoverage](doc/tools/BedLowCoverage.md) - Calcualtes regions of low coverage based on a input BED and BAM file.
* [BedMerge](doc/tools/BedMerge.md) - Merges overlapping regions in a BED file.
* [BedReadCount](doc/tools/BedReadCount.md) - Annoates the regions in a BED file with the read count from a BAM file.
* [BedShrink](doc/tools/BedShrink.md) - Shrinks the regions in a BED file by _n_ bases.
* [BedSort](doc/tools/BedSort.md) - Sorts the regions in a BED file
* [BedSubtract](doc/tools/BedSubtract.md) - Subracts one BED file from another BED file.
* [BedToFasta](doc/tools/BedToFasta.md) - Converts BED file to a FASTA file (based on the reference genome).

### FASTQ tools

* [FastqAddBarcode](doc/tools/FastqAddBarcode.md) - Adds sequences from separate FASTQ as barcodes to read IDs.
* [FastqConvert](doc/tools/FastqConvert.md) - Converts the quality scores from Illumina 1.5 offset to Sanger/Illumina 1.8 offset. 
* [FastqConcat](doc/tools/FastqConcat.md) - Concatinates several FASTQ files into one output FASTQ file. 
* [FastqExtract](doc/tools/FastqExtract.md) - Extracts reads from a FASTQ file according to an ID list.
* [FastqExtractBarcode](doc/tools/FastqExtractBarcode.md) - Moves molecular barcodes of reads to a separate file.
* [FastqExtractUMI](doc/tools/FastqExtractUMI.md) - Moves unique moleculare identifier from read sequence to read ID.
* [FastqFormat](doc/tools/FastqFormat.md) - Determines the quality score offset of a FASTQ file.
* [FastqList](doc/tools/FastqList.md) - Lists read IDs and base counts.
* [FastqMidParser](doc/tools/FastqMidParser.md) - Counts the number of occurances of each MID/index/barcode in a FASTQ file.
* [FastqToFasta](doc/tools/FastqToFasta.md) - Converts FASTQ to FASTA format.
* [FastqTrim](doc/tools/FastqTrim.md) - Trims start/end bases from the reads in a FASTQ file.

### VCF tools (small variants)

* [VcfAnnotateFromBed](doc/tools/VcfAnnotateFromBed.md) - Annotates the INFO column of a VCF with data from a BED file.
* [VcfAnnotateFromVcf](doc/tools/VcfAnnotateFromVcf.md) - Annotates the INFO column of a VCF with data from another VCF file (or multiple VCF files if config file is provided)
* [VcfBreakMulti](doc/tools/VcfBreakMulti.md) - Breaks multi-allelic variants into several lines, making sure that allele-specific INFO/SAMPLE fields are still valid.
* [VcfCheck](doc/tools/VcfCheck.md) - Checks a VCF file for errors.
* [VcfExtractSamples](doc/tools/VcfExtractSamples.md) - Extract one or several samples from a VCF file.
* [VcfFilter](doc/tools/VcfFilter.md) - Filters a VCF based on the given criteria.
* [VcfLeftNormalize](doc/tools/VcfLeftNormalize.md) - Normalizes all variants and shifts indels to the left in a VCF file.
* [VcfSort](doc/tools/VcfSort.md) - Sorts variant lists according to chromosomal position.
* [VcfStreamSort](doc/tools/VcfStreamSort.md) - Sorts entries of a VCF file according to genomic position using a stream.
* [VcfToBedpe](doc/tools/VcfToBedpe.md) - Converts a VCF file containing structural variants to BEDPE format.
* [VcfToTsv](doc/tools/VcfToTsv.md) - Converts a VCF file to a tab-separated text file.


### BEDPE tools (structural variants)

* [BedpeAnnotateFromBed](doc/tools/BedpeAnnotateFromBed.md) - Annotates a BEDPE file with information from a BED file.
* [BedpeGeneAnnotation](doc/tools/BedpeGeneAnnotation.md) - Annotates a BEDPE file with gene information from the NGSD (needs [NGSD](doc/install_ngsd.md)).

### Gene handling tools

* [CnvGeneAnnotation](doc/tools/CnvGeneAnnotation.md) - Annotates TSV file containing CNVs with gene information from NGSD (needs [NGSD](doc/install_ngsd.md)).
* [GenesToApproved](doc/tools/GenesToApproved.md) - Replaces gene symbols by approved symbols using the HGNC database (needs [NGSD](doc/install_ngsd.md)).
* [GenesToBed](doc/tools/GenesToBed.md) - Converts a text file with gene names to a BED file (needs [NGSD](doc/install_ngsd.md)).
* [NGSDExportGenes](doc/tools/NGSDExportGenes.md) - Lists genes from NGSD (needs [NGSD](doc/install_ngsd.md)).

### Misc tools

* [PERsim](doc/tools/PERsim.md) - Paired-end read simulator for Illumina reads.
* [FastaInfo](doc/tools/FastaInfo.md) - Basic info on a FASTA file.
* [NGSDAnnotateCNV](doc/tools/NGSDAnnotateCNV.md) - Annotates a CNV file with overlaping pathogenic CNVs from NGSD (needs [NGSD](doc/install_ngsd.md)).
* [NGSDExportAnnotationData](doc/tools/NGSDExportAnnotationData.md) - Generates a VCF file with all variants and annotations from the NGSD and a BED file containing the gene information of the NGSD (needs [NGSD](doc/install_ngsd.md)).
* [NGSDExportCnvTrack](doc/tools/NGSDExportCnvTrack.md) - Exports a IGV-conform CNV track for a processing system (needs [NGSD](doc/install_ngsd.md)).
* [NGSDExportSamples](doc/tools/NGSDExportSamples.md) - Lists processed samples from the NGSD (needs [NGSD](doc/install_ngsd.md)).
