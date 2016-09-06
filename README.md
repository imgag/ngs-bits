# *ngs-bits* - Short-read sequencing tools


## Obtaining ngs-bits

There are no binary releases yet.  
Please use git to download the most recent development version:

    git clone --recursive https://github.com/imgag/ngs-bits.git

### Resolving proxy issues with git

If you are behind a proxy that block the standard git port, you see something like this:

    $ git clone --recursive https://github.com/imgag/ngs-bits.git
    Cloning into 'ngs-bits'...
    fatal: Unable to look up github.com (port 9418) (Name or service not known)

Then you have to adapt your ~/.gitconfig file like that:

    [http]
    proxy = http://[user]:[password]@[host]:[port]


## Building ngs-bits

### Dependencies

ngs-bits depends on the following software to be installed

* _g++_
* _qmake_ (Qt 5.3 or higher, including xmlpatterns and mysql package)
* _git_ (to extract the version hash)
* _cmake_ (to build the [bamtools](https://github.com/pezmaster31/bamtools) library)
* __optional:__ python and matplotlib (for plot generation in QC tools)

For example, the installation of the dependencies using Ubuntu 14.04/16.04 looks like that:

	> sudo apt-get install g++ qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql git cmake python python-matplotlib

### Building

Just execute the following make commands:

    make build_3rdparty
	make build_tools_release

Now the executables and all required libraries can be found in the bin/ folder!

**Note:** Instructions how to build *ngs-bits* unter Windows can be found [here](doc/install_win.md).

## Support

Please report any issues or questions to the [ngs-bits issue 
tracker](https://github.com/marc-sturm/ngs-bits/issues).

## Tools list

_ngs-bits_ contains a lot of tools that we use for NGS short-read data analysis in our [institute](http://www.uni-tuebingen.de/Klinische_Genetik/start.html). Not all of these tools are mature enough for public use. Thus, here we will list tools that can be safely used:

### Main tools

* [SeqPurge](doc/tools/SeqPurge.md) - A highly-sensitive adapter trimmer for paired-end short-read data ([paper](http://bmcbioinformatics.biomedcentral.com/articles/10.1186/s12859-016-1069-7), [poster](doc/data/SeqPurge_poster.pdf) from ECCB 2016 with more recent benchmarks).
* [SampleCorrelation](doc/tools/SampleCorrelation.md) - Calculates the variant overlap and correlation of two VCF/BAM files.
* [SampleGender](doc/tools/SampleGender.md) - Determines sample gender based on a BAM file.
* [PERsim](doc/tools/PERsim.md) - Paired-end read simulator for Illumina reads.

### QC tools

The default output format of the quality control tools is [qcML](https://github.com/HUPO-PSI/qcML-development/), an XML-based format for -omics quality control, that consists of an [XML schema](https://github.com/HUPO-PSI/qcML-development/blob/master/schema/v0_0_8/qcML_0.0.8.xsd), which defined the overall structure of the format, and an [ontology](https://github.com/HUPO-PSI/qcML-development/blob/master/cv/qc-cv.obo) which defines the QC metrics that can be used.

* [ReadQC](doc/tools/ReadQC.md) - Quality control tool for FASTQ files.
* [MappingQC](doc/tools/MappingQC.md) - Quality control tool for a BAM file.
* [VariantQC](doc/tools/VariantQC.md) - Quality control tool for a VCF file.
* [SomaticQC](doc/tools/SomaticQC.md) - Quality control tool for tumor-normal pairs. Examplary data for download: [tumor-normal.zip](doc/data/somatic_qc.zip?raw=true).

### BAM tools

* [BamClipOverlap](doc/tools/BamClipOverlap.md) - (Soft-)Clips paired-end reads that overlap.
* [BamDownsample](doc/tools/BamDownsample.md) - Downsamples a BAM file to the given percentage of reads.
* [BamIndex](doc/tools/BamIndex.md) - Creates a BAI index for a BAM file.
* [BamLeftAlign](doc/tools/BamLeftAlign.md) - Left-aligns indels in repeat regions.
* [BamToFastq](doc/tools/BamToFastq.md) - Converts a BAM file to FASTQ files (paired-end only).

### BED tools

* [BedAdd](doc/tools/BedAdd.md) - Adds the regions in two BED files.
* [BedAnnotateGC](doc/tools/BedAnnotateGC.md) - Annnotates the regions in a BED file with GC content.
* [BedChunk](doc/tools/BedChunk.md) - Splits regions in a BED file to chunks of a desired size.
* [BedCoverage](doc/tools/BedCoverage.md) - Annotates the regions in a BED file with the average coverage in one or several BAM files.
* [BedExtend](doc/tools/BedExtend.md) - Extends the regions in a BED file by _n_ bases.
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

* [FastqConvert](doc/tools/FastqConvert.md) - Converts the quality scores from Illumina 1.5 offset to Sanger/Illumina 1.8 offset. 
* [FastqExtract](doc/tools/FastqExtract.md) - Extracts reads from a FASTQ file according to an ID list.
* [FastqFormat](doc/tools/FastqFormat.md) - Determines the quality score offset of a FASTQ file.
* [FastqList](doc/tools/FastqList.md) - Lists read IDs and base counts.
* [FastqMidParser](doc/tools/FastqMidParser.md) - Counts the number of occurances of each MID/index/barcode in a FASTQ file.
* [FastqToFasta](doc/tools/FastqToFasta.md) - Converts FASTQ to FASTA format.
* [FastqTrim](doc/tools/FastqTrim.md) - Trims start/end bases from the reads in a FASTQ file.

### VCF tools

* [VariantFilterRegions](doc/tools/VariantFilterRegions.md) - Filter a variant list based on a target region.
* [VcfLeftNormalize](doc/tools/VcfLeftNormalize.md) - Normalizes all variants and shifts indels to the left in a VCF file.
* [VcfSort](doc/tools/VcfSort.md) - Sorts variant lists according to chromosomal position.
* [VcfStreamSort](doc/tools/VcfStreamSort.md) - Sorts entries of a VCF file according to genomic position using a stream.


  










