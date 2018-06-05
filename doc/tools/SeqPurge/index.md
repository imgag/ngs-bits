# SeqPurge documentation

SeqPurge is a highly sensitive adapter trimmer for paired-end short read data.
 
## Using SeqPurge

Using SeqPurge is pretty intuituve. This example shows the use when trimming and merging data of a sample that was sequenced on two lanes:

	> SeqPurge -in1 R1_L1.fastq.gz R1_L2.fastq.gz -in2 R2_L1.fastq.gz R2_L2.fastq.gz -out1 R1.fastq.gz -out2 R2.fastq.gz

The main parameters of SeqPurge are:

- **a1** - Forward read adapter sequence.
- **a2** - Reverse read adapter sequence.
- **mep** - Maximum error probability of insert and adapter matches - this is the main parameter that balances seensitivity and specificity.
- **qcut** - Quality trimming cutoff.
- **ncut** - Number of subsequent Ns to trimmed.

## More information

SeqPurge was published as a [full paper](http://bmcbioinformatics.biomedcentral.com/articles/10.1186/s12859-016-1069-7) in BMC Bioinformatics.

Additionally, there is a [poster presented at ECCB 2016](SeqPurge_poster.pdf) availiable, which contains the latest benchmarks.

## Help and ChangeLog

The SeqPurge command-line help and changelog can be found [here](../SeqPurge.md).

[back to ngs-bits](https://github.com/imgag/ngs-bits)



