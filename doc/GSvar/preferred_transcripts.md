##Preferred transcripts

Some genes have many different transcripts, but in most cases only one of the transcripts is relevant for diagnostics.  
Therefor, a preferred transcript can be defined for each gene.

The preferred transcript is used for:

* Automatically select the preferred transcript in the variant details list.
* Listing only the the preferred transcript in the variant details of the report.

###Setting preferred transcripts

Setting preferred transcripts is only possible by a batch import using the menu entry `Tools > Genes > Preferred trascripts > Import`.  
The import does not give any feedback unless an error occurs. 

*Details information for administrators:  
For this to succeed, the preferred transcripts file has to be configured in the GSvar INI file.  
The file is a tab-separated file with the gene name in first column and NM number (including dot and version) in the third column.*

###Listing preferred transcripts

Preferred transcripts can be listed using the menu entry `Tools > Genes > Preferred trascripts > Show`:

![alt text](preferred_transcripts_list.png)


 

--
[back to main page](index.md)



