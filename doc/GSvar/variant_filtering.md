## Small variant analysis

The data format for small variants (SNVs and InDels) is the `GSvar format`.  
It contains the variants, annotations of the variants and meta data.  
Detailled documentation of the format can be found [here](gsvar_format.md).  

The GSvar application provides default filters, for the most common filter operations.  
These filters can be further modified according to user needs.  
Additionally, powerful custom filters can be created by combining several filter steps.  
A documentation of available filter steps and their parameters can be found [here](https://github.com/imgag/ngs-bits/blob/master/doc/tools/VariantFilterAnnotations.md).

### Single-sample analysis

To perform a single sample analysis, follow those steps:

1. Open the analysis status dialog from the main tool bar ![alt text](analysis_status.png) and trigger the analysis using the single sample button ![alt text](single_sample.png).
2. When the analysis is finished, open the GSvar variant list.
3. For filtering the variant list, use one of the default `recessive` and `dominant` filters.
4. Default filter can be modified and new filters created using the filter toolbar on the right.

#### Mosaic variants

Mosaic variants down to 1% allele frequency are called by default.  
They are contained in the main variant list, but are flagged with the filter entry `mosaic`.  
They are called on the following target region, depending on the processing system:
  
  - WGS: exons regions plus/minus 20 bases.
  - WES: target region of the processing system.
  - panel: target region of the processing system.

Mosaic variants are filtered out by most default filters.  
Use the filters 'mosaic WGS' or 'mosaic WES' to look at them specifically.

#### Variants in not uniquely mappable regions

Variant in not uniquely mappable regions, i.e. reads have a mapping quality of 0, are called by default.  
They are contained in the main variant list, but are flagged with the filter entry `low_mappabilty`.  
They are called on the following target region, depending on the processing system:
  
  - WGS: exons regions plus/minus 20 bases.
  - WES: intersection of target region of the processing system and pre-calculated mapping quality 0 region.
  - panel: intersection of target region of the processing system and pre-calculated mapping quality 0 region.

Low mappability variants are filtered out by most default filters.  
Use the filter 'low mappability' to look at them specifically.

### Trio analysis

To perform a trio analysis, follow those steps:

1. Open the analysis status dialog from the main tool bar ![alt text](analysis_status.png) and trigger the trio analysis using the trio button ![alt text](trio.png).  
2. When the analysis is finished, a folder with the prefix 'Trio_' and the sample names was created.  
Open the GSvar variant list from the trio folder.
3. For filtering the trio variant list, use one of the default `trio` filters.
4. Default filter can be modified and new filters created using the filter toolbar on the right.

**Note:**  
Before performing a trio analysis, always have a look at the index case as a single sample.  
It is possible to miss the causal variant if performing multi-sample analysis only.  
This has both technical and biological reasons (low coverage in one sample, reduced penetrance, mosaic variants,...).


### Multi-sample analysis

To perform a multi-sample analysis, follow those steps:

1. Open the analysis status dialog from the main tool bar ![alt text](analysis_status.png) and trigger the multi-sample analysis using the trio button ![alt text](multi.png).  
2. When the analysis is finished, a folder with the prefix 'Multi_' and the sample names was created in the project folder.  
Open the GSvar variant list from the trio folder.
3. For filtering the trio variant list, use the default `multi-sample` filters.
4. Default filter can be modified and new filters created using the filter toolbar on the right.

**Note:**  
Before performing a multi-sample analysis, always have a look at the index cases as single samples.  
It is possible to miss the causal variant if performing multi-sample analysis only.  
This has both technical and biological reasons (low coverage in one sample, reduced penetrance, mosaic variants, ...).

--

[back to main page](index.md)
