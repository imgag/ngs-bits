#include "TestFramework.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "Settings.h"

TEST_CLASS(NGSHelper_Test)
{
Q_OBJECT
private slots:

	void getKnownVariants()
	{
		VcfFile list = NGSHelper::getKnownVariants("hg19", false);
		I_EQUAL(list.count(), 102467);

		//only SNPs
		list = NGSHelper::getKnownVariants("hg19", true);
		I_EQUAL(list.count(), 97469);

		//only SNPs, AF<80%
		list = NGSHelper::getKnownVariants("hg19", true, 0.0, 0.8);
		I_EQUAL(list.count(), 91186);

		//only SNPs, AF>20%
		list = NGSHelper::getKnownVariants("hg19", true, 0.2);
		I_EQUAL(list.count(), 36022);

		//only SNPs, AF>20%, AF<80%
		list = NGSHelper::getKnownVariants("hg19", true, 0.2, 0.8);
		I_EQUAL(list.count(), 29739);

		//only SNPs on chrX
		BedFile roi_chrx("chrX", 1, 155270560);
		list = NGSHelper::getKnownVariants("hg19", true, roi_chrx, 0.0, 1.0);
		I_EQUAL(list.count(), 1948);
	}

	void getKnownVariants_hg38()
	{
		VcfFile list = NGSHelper::getKnownVariants("hg38", false);
		I_EQUAL(list.count(), 100779);

		//only SNPs, AF<50% on chrX
		BedFile roi_chrx("chrX", 1, 155270560);
		list = NGSHelper::getKnownVariants("hg38", true, roi_chrx, 0.0, 0.5);
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

		NGSHelper::softClipAlignment(al,317587,317596);
		S_EQUAL(al.cigarDataAsString(), "10S123M13I5M");

		//second soft-clip same al
		NGSHelper::softClipAlignment(al,317597,317721);
		S_EQUAL(al.cigarDataAsString(), "148S3M");

		//next alignment
		reader.getNextAlignment(al);
		while(al.isUnmapped())
		{
			reader.getNextAlignment(al);
		}

		//third soft-clip different al1
		NGSHelper::softClipAlignment(al,1048687,1048692);
		NGSHelper::softClipAlignment(al,1048540,1048576);
		S_EQUAL(al.cigarDataAsString(), "36S109M6S");
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
		BedFile par = NGSHelper::pseudoAutosomalRegion("hg19");
		I_EQUAL(par.count(), 4);
		I_EQUAL(par.baseCount(), 5938074);
	}

	void cytoBand()
	{
		S_EQUAL(NGSHelper::cytoBand("chrY", 34847524), "Yq12");
		S_EQUAL(NGSHelper::cytoBand("chr1", 76992611), "1p31.1");
	}

	void cytoBandToRange()
	{
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange(""));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange("Zr36.33"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange("1r36.33"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange("1p36.33-"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange("1p36.33-5q21.2"));
		IS_THROWN(ArgumentException, NGSHelper::cytoBandToRange("1p36.33-1p36.32-1p36.31"));

		S_EQUAL(NGSHelper::cytoBandToRange("chr1p36.33").toString(true), "chr1:1-2300000");
		S_EQUAL(NGSHelper::cytoBandToRange("1p36.33").toString(true), "chr1:1-2300000");
		S_EQUAL(NGSHelper::cytoBandToRange("1p36.33-1p36.32").toString(true), "chr1:1-5400000");
		S_EQUAL(NGSHelper::cytoBandToRange("1p36.32-1p36.33").toString(true), "chr1:1-5400000");
	}

	void impringGenes()
	{
		QMap<QByteArray, ImprintingInfo> imp_genes = NGSHelper::imprintingGenes();

		I_EQUAL(imp_genes.count(), 243);
		S_EQUAL(imp_genes["NPAP1"].source_allele, "paternal");
		S_EQUAL(imp_genes["NPAP1"].status, "imprinted");
		S_EQUAL(imp_genes["NTM"].source_allele, "maternal");
		S_EQUAL(imp_genes["NTM"].status, "imprinted");
		S_EQUAL(imp_genes["SALL1"].source_allele, "maternal");
		S_EQUAL(imp_genes["SALL1"].status, "predicted");
	}
};
