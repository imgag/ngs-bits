TEMPLATE = subdirs
CONFIG += console

#Library targets and depdendencies
SUBDIRS = cppCORE\
        cppXML \
        cppNGS \
        cppNGSD

cppXML.depends = cppCORE
cppNGS.depends = cppCORE cppXML
cppNGSD.depends = cppCORE
cppNGSD.depends = cppNGS

SUBDIRS += tools-TEST
tools-TEST.depends = cppXML cppNGS

SUBDIRS += FastqDemultiplex
tools-TEST.depends += FastqDemultiplex
FastqDemultiplex.depends = cppNGS

SUBDIRS += SamplesNGSD
tools-TEST.depends += SamplesNGSD
SamplesNGSD.depends = cppNGSD

SUBDIRS += BedChunk
tools-TEST.depends += BedChunk
BedChunk.depends = cppNGS

SUBDIRS += BamClipOverlap
tools-TEST.depends += BamClipOverlap
BamClipOverlap.depends = cppNGS

SUBDIRS += BedAnnotateGenes
tools-TEST.depends += BedAnnotateGenes
BedAnnotateGenes.depends = cppNGS

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

SUBDIRS += Cidx
tools-TEST.depends += Cidx
Cidx.depends = cppNGS

SUBDIRS += FastaInfo
tools-TEST.depends += FastaInfo
FastaInfo.depends = cppNGS

SUBDIRS += SampleCorrelation
tools-TEST.depends += SampleCorrelation
SampleCorrelation.depends = cppNGS

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
GenesToApproved.depends = cppNGS

SUBDIRS += GenesToBed
tools-TEST.depends += GenesToBed
GenesToBed.depends = cppNGS

SUBDIRS += VariantFilterRegions
tools-TEST.depends += VariantFilterRegions
VariantFilterRegions.depends = cppNGS

SUBDIRS += BamIndex
tools-TEST.depends += BamIndex
BamIndex.depends = cppNGS

SUBDIRS += VariantAnnotateFrequency
tools-TEST.depends += VariantAnnotateFrequency
VariantAnnotateFrequency.depends = cppNGS

SUBDIRS += TrioAnnotation
tools-TEST.depends += TrioAnnotation
TrioAnnotation.depends = cppNGS

SUBDIRS += MappingQC
tools-TEST.depends += MappingQC
MappingQC.depends = cppNGS

SUBDIRS += VariantQC
tools-TEST.depends += VariantQC
VariantQC.depends = cppNGS

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

SUBDIRS += BamLeftAlign
tools-TEST.depends += BamLeftAlign
BamLeftAlign.depends = cppNGS

SUBDIRS += BedToFasta
tools-TEST.depends += BedToFasta
BedToFasta.depends = cppNGS

SUBDIRS += VcfLeftAlign
tools-TEST.depends += VcfLeftAlign
VcfLeftAlign.depends = cppNGS

SUBDIRS += FastqMidParser
tools-TEST.depends += FastqMidParser
FastqMidParser.depends = cppNGS

SUBDIRS += FastqTrim
tools-TEST.depends += FastqTrim
FastqTrim.depends = cppNGS

SUBDIRS += FastqConvert
tools-TEST.depends += FastqConvert
FastqConvert.depends = cppNGS

SUBDIRS += VariantAnnotateNGSD
tools-TEST.depends += VariantAnnotateNGSD
VariantAnnotateNGSD.depends = cppNGSD

SUBDIRS += EstimateTumorContent
tools-TEST.depends += EstimateTumorContent
EstimateTumorContent.depends = cppNGS

SUBDIRS += SampleOverview
tools-TEST.depends += SampleOverview
SampleOverview.depends = cppNGS

SUBDIRS += CnvHunter
tools-TEST.depends += CnvHunter
CnvHunter.depends = cppNGS

SUBDIRS += BedGeneOverlap
tools-TEST.depends += BedGeneOverlap
BedGeneOverlap.depends = cppNGS

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

SUBDIRS += NGSDInit
tools-TEST.depends += NGSDInit
NGSDInit.depends = cppNGSD

#other stuff
OTHER_FILES += ToDos.txt

