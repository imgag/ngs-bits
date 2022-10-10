#include "TestFramework.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"
#include <iostream>

TEST_CLASS(NGSHelper_Test)
{
Q_OBJECT
private slots:

	void getKnownVariants()
	{
		VcfFile list = NGSHelper::getKnownVariants(GenomeBuild::HG19, false);
		I_EQUAL(list.count(), 102467);

		//only SNPs
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true);
		I_EQUAL(list.count(), 97469);

		//only SNPs, AF<80%
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, 0.0, 0.8);
		I_EQUAL(list.count(), 91186);

		//only SNPs, AF>20%
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, 0.2);
		I_EQUAL(list.count(), 36022);

		//only SNPs, AF>20%, AF<80%
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, 0.2, 0.8);
		I_EQUAL(list.count(), 29739);

		//only SNPs on chrX
		BedFile roi_chrx("chrX", 1, 155270560);
		list = NGSHelper::getKnownVariants(GenomeBuild::HG19, true, roi_chrx, 0.0, 1.0);
		I_EQUAL(list.count(), 1948);
	}

	void getKnownVariants_hg38()
	{
		VcfFile list = NGSHelper::getKnownVariants(GenomeBuild::HG38, false);
		I_EQUAL(list.count(), 100779);

		//only SNPs, AF<50% on chrX
		BedFile roi_chrx("chrX", 1, 155270560);
		list = NGSHelper::getKnownVariants(GenomeBuild::HG38, true, roi_chrx, 0.0, 0.5);
		I_EQUAL(list.count(), 1548);
	}

	void softClipAlignment()
	{
		BamReader reader(TESTDATA("data_in/panel.bam"));
		BamAlignment al;

		//first soft-clip al
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		NGSHelper::softClipAlignment(al,146992, 147014);
		S_EQUAL(al.cigarDataAsString(), "23S128M");

		//second soft-clip same al
		NGSHelper::softClipAlignment(al,147015,147139);
		S_EQUAL(al.cigarDataAsString(), "148S3M");

		//next alignment
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}
		//third soft-clip different al1
		NGSHelper::softClipAlignment(al,147234,147269);
		NGSHelper::softClipAlignment(al,147378,147384);
		S_EQUAL(al.cigarDataAsString(), "36S108M7S");
	}

	void softClipAlignment2()
	{
		BamReader reader(TESTDATA("data_in/bamclipoverlap.bam"));
		BamAlignment al;

		//first soft-clip al
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:1101:2110:14905");
		NGSHelper::softClipAlignment(al,33038615,33038624);
		S_EQUAL(al.cigarDataAsString(), "5H10S141M");
		I_EQUAL(al.start(), 33038625);

		NGSHelper::softClipAlignment(al,33038756,33038765);
		S_EQUAL(al.cigarDataAsString(), "5H10S131M10S");

		reader.getNextAlignment(al);
		reader.getNextAlignment(al);

		S_EQUAL(al.name(), "PC0226:55:000000000-A5CV9:1:1101:2110:14905");
		NGSHelper::softClipAlignment(al,33038659,33038668);
		S_EQUAL(al.cigarDataAsString(), "10S141M5H");
		I_EQUAL(al.start(), 33038669);

		NGSHelper::softClipAlignment(al,33038800,33038809);
		S_EQUAL(al.cigarDataAsString(), "10S131M10S5H");
	}

	void pseudoAutosomalRegion()
	{
		BedFile par = NGSHelper::pseudoAutosomalRegion(GenomeBuild::HG19);
		I_EQUAL(par.count(), 4);
		I_EQUAL(par.baseCount(), 5938074);
	}

	void cytoBand()
	{
		S_EQUAL(NGSHelper::cytoBand(GenomeBuild::HG19, "chrY", 34847524), "Yq12");
		S_EQUAL(NGSHelper::cytoBand(GenomeBuild::HG19, "chr1", 76992611), "1p31.1");
	}

	void cytoBandToRange()
	{
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, ""));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "Zr36.33"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1r36.33"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-5q21.2"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-1p36.32-1p36.31"));

		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "chr1p36.33").toString(true), "chr1:1-2300000");
		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33").toString(true), "chr1:1-2300000");
		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.33-1p36.32").toString(true), "chr1:1-5400000");
		S_EQUAL(NGSHelper::cytoBandToRange(GenomeBuild::HG19, "1p36.32-1p36.33").toString(true), "chr1:1-5400000");
	}

	void impringGenes()
	{
		QMap<QByteArray, ImprintingInfo> imp_genes = NGSHelper::imprintingGenes();

		I_EQUAL(imp_genes.count(), 247);
		S_EQUAL(imp_genes["NPAP1"].expressed_allele, "paternal");
		S_EQUAL(imp_genes["NPAP1"].status, "imprinted");
		S_EQUAL(imp_genes["NTM"].expressed_allele, "maternal");
		S_EQUAL(imp_genes["NTM"].status, "imprinted");
		S_EQUAL(imp_genes["SALL1"].expressed_allele, "maternal");
		S_EQUAL(imp_genes["SALL1"].status, "predicted");
	}

	void centromeres()
	{
		BedFile centros = NGSHelper::centromeres(GenomeBuild::HG19);
		I_EQUAL(centros.count(), 24);
		S_EQUAL(centros[1].toString(true), "chr2:92326171-95326171");
		S_EQUAL(centros[11].toString(true), "chr12:34856694-37856694");

		BedFile centros4 = NGSHelper::centromeres(GenomeBuild::HG38);
		I_EQUAL(centros4.count(), 24);
		S_EQUAL(centros4[0].toString(true), "chr1:121700000-125100000");
	}

	void telomeres()
	{
		BedFile telos1 = NGSHelper::telomeres(GenomeBuild::HG19);
		I_EQUAL(telos1.count(), 46);
		S_EQUAL(telos1[45].toString(true), "chrY:59363566-59373566");

		BedFile telos2 = NGSHelper::telomeres(GenomeBuild::HG38);
		I_EQUAL(telos2.count(), 48);
		S_EQUAL(telos2[32].toString(true), "chr17:1-10000");
		S_EQUAL(telos2[45].toString(true), "chrX:156030895-156040895");
	}

	void populationCodeToHumanReadable()
	{
		S_EQUAL(NGSHelper::populationCodeToHumanReadable(""), "");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("EUR"), "European");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("AFR"), "African");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("SAS"), "South asian");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("EAS"), "East asian");
		S_EQUAL(NGSHelper::populationCodeToHumanReadable("ADMIXED/UNKNOWN"), "Admixed/Unknown");
	}

	void transcriptMatches()
	{
		//HG19
		auto matches = NGSHelper::transcriptMatches(GenomeBuild::HG19);
		IS_FALSE(matches.contains("ENST00000644374"));
		IS_TRUE(matches.contains("ENST00000004921"));
		I_EQUAL(matches["ENST00000004921"].count(), 2);
		IS_TRUE(matches["ENST00000004921"].contains("CCDS11306"));
		IS_TRUE(matches["ENST00000004921"].contains("NM_002988"));
		IS_TRUE(matches["CCDS11306"].contains("ENST00000004921"));
		IS_TRUE(matches["NM_002988"].contains("ENST00000004921"));

		//HG38
		matches = NGSHelper::transcriptMatches(GenomeBuild::HG38);
		IS_TRUE(matches.contains("ENST00000644374"));
		IS_FALSE(matches.contains("ENST00000004921"));
		I_EQUAL(matches["ENST00000644374"].count(), 2);
		IS_TRUE(matches["ENST00000644374"].contains("NM_004447"));
		IS_TRUE(matches["ENST00000644374"].contains("CCDS31753"));
		IS_TRUE(matches["CCDS31753"].contains("ENST00000644374"));
		IS_TRUE(matches["NM_004447"].contains("ENST00000644374"));
	}
};
