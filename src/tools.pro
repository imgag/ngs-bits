TEMPLATE = subdirs
CONFIG += console

#Library targets and depdendencies
SUBDIRS = cppCORE\
        cppXML \
        cppNGS \
        cppNGSD
cppXML.depends = cppCORE
cppNGS.depends = cppXML
cppNGSD.depends = cppNGS

SUBDIRS += tools-TEST
tools-TEST.depends = cppNGSD

#tools depending on cppNGS
TOOLS_NGS = \
	BamCleanHaloplex \ 
	BamClipOverlap \ 
	BamDownsample \ 
	BamExtract \ 
	BamFilter \ 
	BamInfo \ 
	BamRemoveVariants \ 
	BamToFastq \ 
	BedAdd \ 
	BedAnnotateFreq \ 
	BedAnnotateFromBed \ 
	BedAnnotateGC \ 
	BedChunk \ 
	BedCoverage \ 
	BedExtend \ 
	BedHighCoverage \ 
	BedInfo \ 
	BedIntersect \ 
	BedLiftOver \ 
	BedLowCoverage \ 
	BedMerge \ 
	BedReadCount \ 
	BedSort \ 
	BedSubtract \ 
	BedToEpigen \ 
	BedToFasta \ 
	BedShrink \ 
	BedpeAnnotateBreakpointDensity \ 
	BedpeAnnotateCnvOverlap \ 
	BedpeAnnotateFromBed \ 
	BedpeExtractGenotype \ 
	BedpeExtractInfoField \ 
	BedpeFilter \ 
	BedpeSort \ 
	BedpeToBed \ 
	CfDnaQC \ 
	CnvFilterAnnotations \ 
	CnvReferenceCohort \ 
	ExtractMethylationData \ 
	FastaChecksumUpdate \ 
	FastaFromBam \ 
	FastaInfo \ 
	FastaMask \ 
	FastqAddBarcode \ 
	FastqCheckUMI \ 
	FastqConcat \ 
	FastqConvert \ 
	FastqDownsample \ 
	FastqExtract \ 
	FastqExtractBarcode \ 
	FastqExtractUMI \ 
	FastqFormat \ 
	FastqList \ 
	FastqMidParser \ 
	FastqToFasta \ 
	FastqTrim \ 
	GenePrioritization \ 
	GenlabInfo \ 
	GraphStringDb \ 
	MantaVcfFix \ 
	MappingQC \ 
	NGSDImportGenlab \ 
	NGSDImportOncotree \ 
	NgsBitsInfo \ 
	QcToTsv \ 
	ReadQC \ 
	RnaQC \ 
	RohHunter \ 
	SampleAncestry \ 
	SampleGender \ 
	SampleIdentity \ 
	SampleSimilarity \ 
	SeqPurge \ 
	SvFilterAnnotations \ 
	TranscriptToProtein \ 
	TrioMaternalContamination \ 
	TrioMendelianErrors \ 
	TsvAnnotate \ 
	TsvDiff \ 
	TsvFilter \ 
	TsvInfo \ 
	TsvMerge \ 
	TsvSlice \ 
	TsvTo \ 
	TsvToQC \ 
	UpdHunter \ 
	VariantAnnotateASE \ 
	VariantAnnotateFrequency \ 
	VariantFilterAnnotations \ 
	VariantFilterRegions \ 
	VariantQC \ 
	VcfAdd \ 
	VcfAnnotateConsequence \ 
	VcfAnnotateFrequency \ 
	VcfAnnotateFromBed \ 
	VcfAnnotateFromBigWig \ 
	VcfAnnotateFromVcf \ 
	VcfAnnotateHexplorer \ 
	VcfBreakMulti \ 
	VcfCalculatePRS \ 
	VcfCheck \ 
	VcfExtractSamples \ 
	VcfFilter \ 
	VcfLeftNormalize \ 
	VcfMerge \ 
	VcfReplaceSamples \ 
	VcfSort \ 
	VcfSplit \ 
	VcfStreamSort \ 
	VcfStrip \ 
	VcfSubtract \ 
	VcfToBed \ 
	VcfToBedpe \ 
	VcfToTsv \ 

#tools depending on cppNGSD
TOOLS_NGSD = \
	BedAnnotateGenes \ 
	BedAnnotateGenes \ 
	BedGeneOverlap \ 
	BedpeAnnotateCounts \ 
	BedpeGeneAnnotation \ 
	CnvGeneAnnotation \ 
	ExportcBioportal \ 
	GenesToApproved \ 
	GenesToBed \ 
	GenesToTranscripts \ 
	HgvsToVcf \ 
	NGSDAddVariantsGermline \ 
	NGSDAddVariantsSomatic \ 
	NGSDAnnotateCNV \ 
	NGSDAnnotateGeneExpression \ 
	NGSDAnnotateRNA \ 
	NGSDAnnotateSV \ 
	NGSDExportAnnotationData \ 
	NGSDExportCnvTrack \ 
	NGSDExportGenes \ 
	NGSDExportGff \ 
	NGSDExportIgvGeneTrack \ 
	NGSDExportSV \ 
	NGSDExportSamples \ 
	NGSDExportSpliceAI \ 
	NGSDExportStudyGHGA \ 
	NGSDExtractRNACohort \ 
	NGSDGeneBurdenTest \ 
	NGSDImportCSpec \ 
	NGSDImportClinvarAccessions \ 
	NGSDImportEnsembl \ 
	NGSDImportExpressionData \ 
	NGSDImportGeneInfo \ 
	NGSDImportHGNC \ 
	NGSDImportHPO \ 
	NGSDImportOMIM \ 
	NGSDImportORPHA \ 
	NGSDImportQC \ 
	NGSDImportSampleQC \ 
	NGSDInit \ 
	NGSDSameSample \ 
	NGSDSampleUsers \ 
	NGSDTransferReportConfig \ 
	PhenotypeSubtree \ 
	PhenotypesToGenes \ 
	SamplePath \ 
	SnifflesVcfFix \ 
	SomaticQC \ 
	SplicingToBed \ 
	TranscriptComparison \ 
	TranscriptsToBed \ 
	VariantRanking \ 
	VcfAnnotateMaxEntScan \ 

SUBDIRS += $$TOOLS_NGS $$TOOLS_NGSD
tools-TEST.depends = $$TOOLS_NGS $$TOOLS_NGSD

for(tool, TOOLS_NGS) {
    eval($${tool}.depends = cppNGS)
}

for(tool, TOOLS_NGSD) {
    eval($${tool}.depends = cppNGSD)
}