# *ngs-bits* - Short-read seqencing tools


## Obtaining ngs-bits

There are no binary releases yet.  
Please use git to download the most recent development version:

    git clone --recursive https://github.com/marc-sturm/ngs-bits.git

### Resolving proxy issues with git

If you are behind a proxy that block the standard git port, you see something like this:

    $ git clone --recursive https://github.com/marc-sturm/ngs-bits.git
    Cloning into 'ngs-bits'...
    fatal: Unable to look up github.com (port 9418) (Name or service not known)

Then you have to adapt your ~/.gitconfig file like that:

    [http]
    proxy = http://[user]:[password]@[host]:[port]


## Building ngs-bits

### Dependencies

ngs-bits depends on the following software to be installed

- g++
- qmake (Qt 5.5 or higher)
- git (to extract the version hash)
- cmake (to build bamtools library)

### Building

Just execute the following make commands:

    make build_3rdparty
	make build_tools_release

Now the executables and all required libraries can be found in the bin/ folder!

## Contributors

ngs-bits is developed and maintained by:

- Marc Sturm
- Christopher Schroeder
- Florian Lenz

## Support

Please report any issues or questions to the [ngs-bits issue 
tracker](https://github.com/marc-sturm/ngs-bits/issues).

## Tools list

_ngs-bits_ contains a lot of tools that we use for NGS short-read data analysis in our institute. Not all of these tools are mature enough for public use. Thus, here we will list tools that can be safely used:

### Main tools

 * _SeqPurge_ - A highly-sensitive adapter trimmer for paired-end short-read data ([ISMB 2015 Poster abstract](http://www.iscb.org/cms_addon/conferences/ismbeccb2015/posterlist.php?cat=G)).
 * _ReadQC_ - Quality control tool for FASTQ files (output is [qcML](https://code.google.com/p/qcml/)).
 * _MappingQC_ - Quality control tool for a BAM file (output is [qcML](https://code.google.com/p/qcml/)).
 * _VariantQC_ - Quality control tool for a VCF file (output is [qcML](https://code.google.com/p/qcml/)).
 * _SampleCorrelation_ - Calculates the variant overlap and correlation of two VCF/BAM files.
 * _SampleGender_ - Determines sample gender based on a BAM file.

### BAM tools

 * _BamClipOverlap_ - Clips paired-end reads that overlap.
 * _BamIndex_ - Creates a BAI index for a BAM file.
 * _BamLeftAlign_ - Left-aligns indels in repeat regions.

### BED tools

 * _BedAnnotateGC_ - Annnotates the regions in a BED file with GC content.
 * _BedChunk_ - Splits regions in a BED file to chunks of a desired size.
 * _BedCoverage_ - Annoates the regions in a BED file with the average coverage in one or several BAM files.
 * _BedExtend_ - Extends the regions in a BED file by _n_ bases.
 * _BedInfo_ - Prints summary information about a BED file.
 * _BedIntersect_ - Intersects two BED files.
 * _BedLowCoverage_ - Calcualtes regions of low coverage based on a input BED and BAM file.
 * _BedMerge_ - Merges overlapping regions in a BED file.
 * _BedShrink_ - Shrinks the regions in a BED file by _n_ bases.
 * _BedSort_ - Sorts the regions in a BED file
 * _BedSubtract_ - Subracts one BED file from another BED file.
 * _BedToFasta_ - Converts BED file to a FASTA file (based on the reference genome).

### FASTQ tools

 * _FastqConvert_ - Converts the quality scores from Illumina 1.5 offset to Sanger/Illumina 1.8 offset. 
 * _FastqExtract_ - Extracts reads from a FASTQ file according to an ID list.
 * _FastqFormat_ - Determines the quality score offset of a FASTQ file.
 * _FastqList_ - Lists read IDs and base counts.
 * _FastqMidParser_ - Counts the number of occurances of each MID/index/barcode in a FASTQ file.
 * _FastqToFasta_ - Converts FASTQ to FASTA format.
 * _FastqTrim_ - Trims start/end bases from the reads in a FASTQ file.

### VCF tools

 * _VariantFilterRegions_ - Filter a variant list based on a BED file.
 * _VcfLeftAlign_ - Left-aligns indel variants in repeat regions.
 * _VcfSort_ - Sorts variant lists according to chromosomal position.
  