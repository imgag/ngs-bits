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

SUBDIRS += SampleAncestry
tools-TEST.depends += SampleAncestry
SampleAncestry.depends = cppNGS

SUBDIRS += UpdHunter
tools-TEST.depends += UpdHunter
UpdHunter.depends = cppNGS

SUBDIRS += NGSDImportQC
tools-TEST.depends += NGSDImportQC
NGSDImportQC.depends = cppNGSD

SUBDIRS += NGSDExportGenes
tools-TEST.depends += NGSDExportGenes
NGSDExportGenes.depends = cppNGSD

SUBDIRS += NGSDExportSamples
tools-TEST.depends += NGSDExportSamples
NGSDExportSamples.depends = cppNGSD

SUBDIRS += BedChunk
tools-TEST.depends += BedChunk
BedChunk.depends = cppNGS

SUBDIRS += BamClipOverlap
tools-TEST.depends += BamClipOverlap
BamClipOverlap.depends = cppNGS

SUBDIRS += BedAnnotateGenes
tools-TEST.depends += BedAnnotateGenes
BedAnnotateGenes.depends = cppNGSD

SUBDIRS += BedCoverage
tools-TEST.depends += BedCoverage
BedCoverage.depends = cppNGS

SUBDIRS += BedLowCoverage
tools-TEST.depends += BedLowCoverage
BedLowCoverage.depends = cppNGS

SUBDIRS += BedExtend
tools-TEST.depends += BedExtend
BedExtend.depends = cppNGS

SUBDIRS += BedInfo
tools-TEST.depends += BedInfo
BedInfo.depends = cppNGS

SUBDIRS += BedIntersect
tools-TEST.depends += BedIntersect
BedIntersect.depends = cppNGS

SUBDIRS += BedMerge
tools-TEST.depends += BedMerge
BedMerge.depends = cppNGS

SUBDIRS += BedShrink
tools-TEST.depends += BedShrink
BedShrink.depends = cppNGS

SUBDIRS += BedSort
tools-TEST.depends += BedSort
BedSort.depends = cppNGS

SUBDIRS += BedSubtract
tools-TEST.depends += BedSubtract
BedSubtract.depends = cppNGS

SUBDIRS += FastaInfo
tools-TEST.depends += FastaInfo
FastaInfo.depends = cppNGS

SUBDIRS += SampleSimilarity
tools-TEST.depends += SampleSimilarity
SampleSimilarity.depends = cppNGS

SUBDIRS += SampleDiff
tools-TEST.depends += SampleDiff
SampleDiff.depends = cppNGS

SUBDIRS += SampleGender
tools-TEST.depends += SampleGender
SampleGender.depends = cppNGS

SUBDIRS += BedAnnotateFreq
tools-TEST.depends += BedAnnotateFreq
BedAnnotateFreq.depends = cppNGS

SUBDIRS += GenesToApproved
tools-TEST.depends += GenesToApproved
GenesToApproved.depends = cppNGSD

SUBDIRS += GenesToBed
tools-TEST.depends += GenesToBed
GenesToBed.depends = cppNGSD

SUBDIRS += VariantFilterRegions
tools-TEST.depends += VariantFilterRegions
VariantFilterRegions.depends = cppNGS

SUBDIRS += VariantAnnotateFrequency
tools-TEST.depends += VariantAnnotateFrequency
VariantAnnotateFrequency.depends = cppNGS

SUBDIRS += MappingQC
tools-TEST.depends += MappingQC
MappingQC.depends = cppNGS

SUBDIRS += VariantQC
tools-TEST.depends += VariantQC
VariantQC.depends = cppNGS

SUBDIRS += SomaticQC
tools-TEST.depends += SomaticQC
SomaticQC.depends = cppNGS

SUBDIRS += FastqExtract
tools-TEST.depends += FastqExtract
FastqExtract.depends = cppNGS

SUBDIRS += FastqList
tools-TEST.depends += FastqList
FastqList.depends = cppNGS

SUBDIRS += FastqFormat
tools-TEST.depends += FastqFormat
FastqFormat.depends = cppNGS

SUBDIRS += ReadQC
tools-TEST.depends += ReadQC
ReadQC.depends = cppNGS

SUBDIRS += BedToFasta
tools-TEST.depends += BedToFasta
BedToFasta.depends = cppNGS

SUBDIRS += FastqMidParser
tools-TEST.depends += FastqMidParser
FastqMidParser.depends = cppNGS

SUBDIRS += FastqTrim
tools-TEST.depends += FastqTrim
FastqTrim.depends = cppNGS

SUBDIRS += FastqConvert
tools-TEST.depends += FastqConvert
FastqConvert.depends = cppNGS

SUBDIRS += SampleOverview
tools-TEST.depends += SampleOverview
SampleOverview.depends = cppNGS

SUBDIRS += CnvHunter
tools-TEST.depends += CnvHunter
CnvHunter.depends = cppNGS

SUBDIRS += BedGeneOverlap
tools-TEST.depends += BedGeneOverlap
BedGeneOverlap.depends = cppNGSD

SUBDIRS += SeqPurge
tools-TEST.depends += SeqPurge
SeqPurge.depends = cppNGS

SUBDIRS += VcfToTsv
tools-TEST.depends += VcfToTsv
VcfToTsv.depends = cppNGS

SUBDIRS += VcfSort
tools-TEST.depends += VcfSort
VcfSort.depends = cppNGS

SUBDIRS += TsvInfo
tools-TEST.depends += TsvInfo
TsvInfo.depends = cppNGS

SUBDIRS += TsvSlice
tools-TEST.depends += TsvSlice
TsvSlice.depends = cppNGS

SUBDIRS += TsvFilter
tools-TEST.depends += TsvFilter
TsvFilter.depends = cppNGS

SUBDIRS += BedAnnotateGC
tools-TEST.depends += BedAnnotateGC
BedAnnotateGC.depends = cppNGS

SUBDIRS += FastqToFasta
tools-TEST.depends += FastqToFasta
FastqToFasta.depends = cppNGS

SUBDIRS += BamCleanHaloplex
tools-TEST.depends += BamCleanHaloplex
BamCleanHaloplex.depends = cppNGS

SUBDIRS += BedAdd
tools-TEST.depends += BedAdd
BedAdd.depends = cppNGS

SUBDIRS += BamDownsample
tools-TEST.depends += BamDownsample
BamDownsample.depends = cppNGS

SUBDIRS += BamToFastq
tools-TEST.depends += BamToFastq
BamToFastq.depends = cppNGS

SUBDIRS += NGSDInit
tools-TEST.depends += NGSDInit
NGSDInit.depends = cppNGSD

SUBDIRS += NGSDImportHGNC
tools-TEST.depends += NGSDImportHGNC
NGSDImportHGNC.depends = cppNGSD

SUBDIRS += NGSDImportEnsembl
tools-TEST.depends += NGSDImportEnsembl
NGSDImportEnsembl.depends = cppNGSD

SUBDIRS += BedReadCount
tools-TEST.depends += BedReadCount
BedReadCount.depends = cppNGS

SUBDIRS += NGSDImportHPO
tools-TEST.depends += NGSDImportHPO
NGSDImportHPO.depends = cppNGSD

SUBDIRS += NGSDImportOMIM
tools-TEST.depends += NGSDImportOMIM
NGSDImportOMIM.depends = cppNGSD

SUBDIRS += NGSDImportORPHA
tools-TEST.depends += NGSDImportORPHA
NGSDImportORPHA.depends = cppNGSD

SUBDIRS += FastqExtractBarcode
tools-TEST.depends += FastqExtractBarcode
FastqExtractBarcode.depends = cppNGS

SUBDIRS += PERsim
tools-TEST.depends += PERsim
PERsim.depends = cppNGS

SUBDIRS += VariantFilterAnnotations
tools-TEST.depends += VariantFilterAnnotations
VariantFilterAnnotations.depends = cppNGS

SUBDIRS += VcfLeftNormalize
tools-TEST.depends += VcfLeftNormalize
VcfLeftNormalize.depends = cppNGS

SUBDIRS += VcfStreamSort
tools-TEST.depends += VcfStreamSort
VcfStreamSort.depends = cppNGS

SUBDIRS += NGSDImportGeneInfo
tools-TEST.depends += NGSDImportGeneInfo
NGSDImportGeneInfo.depends = cppNGSD

SUBDIRS += VcfAnnotateFromBed
tools-TEST.depends += VcfAnnotateFromBed
VcfAnnotateFromBed.depends = cppNGS

SUBDIRS += NGSDMaintain
tools-TEST.depends += NGSDMaintain
NGSDMaintain.depends = cppNGSD

SUBDIRS += TsvMerge
tools-TEST.depends += TsvMerge
TsvMerge.depends = cppNGS

SUBDIRS += BedAnnotateFromBed
tools-TEST.depends += BedAnnotateFromBed
BedAnnotateFromBed.depends = cppNGS

SUBDIRS += RohHunter
tools-TEST.depends += RohHunter
RohHunter.depends = cppNGS

SUBDIRS += FastqExtractUMI
tools-TEST.depends += FastqExtractUMI
FastqExtractUMI.depends = cppNGS

SUBDIRS += FastqAddBarcode
tools-TEST.depends += FastqAddBarcode
FastqAddBarcode.depends = cppNGS

SUBDIRS += BamFilter
tools-TEST.depends += BamFilter
BamFilter.depends = cppNGS

SUBDIRS += VcfCheck
tools-TEST.depends += VcfCheck
VcfCheck.depends = cppNGS

SUBDIRS += VcfBreakMulti
tools-TEST.depends += VcfBreakMulti
VcfBreakMulti.depends = cppNGS

SUBDIRS += VcfFilter
tools-TEST.depends += VcfFilter
VcfFilter.depends = cppNGS

SUBDIRS += VcfExtractSamples
tools-TEST.depends += VcfExtractSamples
VcfExtractSamples.depends = cppNGS

SUBDIRS += FastqConcat
tools-TEST.depends += FastqConcat
FastqConcat.depends = cppNGS

SUBDIRS += VcfToBedpe
tools-TEST.depends += VcfToBedpe
VcfToBedpe.depends = cppNGS

SUBDIRS += NGSDAddVariantsGermline
tools-TEST.depends += NGSDAddVariantsGermline
NGSDAddVariantsGermline.depends = cppNGSD

SUBDIRS += NGSDAddVariantsSomatic
tools-TEST.depends += NGSDAddVariantsSomatic
NGSDAddVariantsSomatic.depends = cppNGSD

SUBDIRS += NGSDExportAnnotationData
tools-TEST.depends += NGSDExportAnnotationData
NGSDExportAnnotationData.depends = cppNGSD

SUBDIRS += VcfAnnotateFromVcf
tools-TEST.depends += VcfAnnotateFromVcf
VcfAnnotateFromVcf.depends = cppNGS

SUBDIRS += NGSDExportCnvTrack
tools-TEST.depends += NGSDExportCnvTrack
NGSDExportCnvTrack.depends = cppNGSD

SUBDIRS += CnvGeneAnnotation
tools-TEST.depends += CnvGeneAnnotation
CnvGeneAnnotation.depends = cppNGSD

SUBDIRS += BedpeGeneAnnotation
tools-TEST.depends += BedpeGeneAnnotation
BedpeGeneAnnotation.depends = cppNGSD

SUBDIRS += BedpeAnnotateFromBed
tools-TEST.depends += BedpeAnnotateFromBed
BedpeAnnotateFromBed.depends = cppNGS

SUBDIRS += NGSDAnnotateCNV
tools-TEST.depends += NGSDAnnotateCNV
NGSDAnnotateCNV.depends = cppNGSD

SUBDIRS += NGSDAnnotateSV
tools-TEST.depends += NGSDAnnotateSV
NGSDAnnotateSV.depends = cppNGSD

SUBDIRS += SvFilterAnnotations
tools-TEST.depends += SvFilterAnnotations
SvFilterAnnotations.depends = cppNGS

SUBDIRS += BedpeToBed
tools-TEST.depends += BedpeToBed
BedpeToBed.depends = cppNGS

SUBDIRS += BedHighCoverage
tools-TEST.depends += BedHighCoverage
BedHighCoverage.depends = cppNGS

SUBDIRS += PhenotypesToGenes
tools-TEST.depends += PhenotypesToGenes
PhenotypesToGenes.depends = cppNGSD

SUBDIRS += PhenotypeSubtree
tools-TEST.depends += PhenotypeSubtree
PhenotypeSubtree.depends = cppNGSD

SUBDIRS += CnvFilterAnnotations
tools-TEST.depends += CnvFilterAnnotations
CnvFilterAnnotations.depends = cppNGS

SUBDIRS += BedpeFilter
tools-TEST.depends += BedpeFilter
BedpeFilter.depends = cppNGS

SUBDIRS += BedpeAnnotateCnvOverlap
tools-TEST.depends += BedpeAnnotateCnvOverlap
BedpeAnnotateCnvOverlap.depends = cppNGS

SUBDIRS += TrioMaternalContamination
tools-TEST.depends += TrioMaternalContamination
TrioMaternalContamination.depends = cppNGS

SUBDIRS += FastqDownsample
tools-TEST.depends += FastqDownsample
FastqDownsample.depends = cppNGS

SUBDIRS += VcfCalculatePRS
tools-TEST.depends += VcfCalculatePRS
VcfCalculatePRS.depends = cppNGS

#other stuff
OTHER_FILES += ToDos.txt
