##Variant filtering

### Single-sample analysis

TODO

### Trio analysis

To perform a trio analysis, follow those steps:

1. Trigger the trio analysis using the trio button ![alt text](trio.png) in the main tool bar.  
2. When the analysis is finished, a folder with the prefix 'Trio_' and the sample names exists. Open the GSvar variant lst from the trio folder.
3. For filtering the trio variant list, use the default filter `germline, trio` from the main tool bar.

### Multi-sample analysis

A dedicated analysis pipeline and GSvar support for non-trio multi-sample analysis is currently in development.  
Meanwhile, you can use this workaround:

1. Combine the variant lists from several sample using `Tools > Create sample overview`. This creates a variant overview file with all variants as rows and one column per sample which contains the genotype of the sample for each variant.
2. Load the combined variant list and filter it using the default filter `germline, multi-sample` from the main tool bar.  
3. This filter does not take the affected/unaffected status or the genotype of the individual samples into account. Thus you probably want to copy remaining variants to  Excel and filter them further based on the genotypes.

--
[back to main page](index.md)



