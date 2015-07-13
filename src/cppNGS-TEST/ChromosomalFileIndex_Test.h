#include "TestFramework.h"
#include "ChromosomalFileIndex.h"
#include "VariantList.h"

TEST_CLASS(ChromosomalFileIndex_Test)
{
Q_OBJECT
private slots:

	void filePosition()
	{
		ChromosomalFileIndex index;
		index.load(TESTDATA("data_in/ChromosomalFileIndex.tsv"));

		//find 1
		QPair<long, long> pos = index.filePosition("chr1", 19652900, 21031300);
		I_EQUAL(pos.first, 8704);
		I_EQUAL(pos.second, 8768);

		//find 2
		pos = index.filePosition("chr1", 865650, 865664);
		I_EQUAL(pos.first, 274);
		I_EQUAL(pos.second, 565);
	}

	void lines()
	{
		ChromosomalFileIndex index;
		index.load(TESTDATA("data_in/ChromosomalFileIndex.tsv"));

		//find 1
		QStringList lines = index.lines("chr1", 865650, 865664);
		I_EQUAL(lines.count(), 2);
		S_EQUAL(lines[0], QString("1\t865654\t865654\tC\tT\t0.000116"));
		S_EQUAL(lines[1], QString("1\t865664\t865664\tC\tT\t0.000116"));

		//find 2
		lines = index.lines("chr1", 865654, 865663);
		I_EQUAL(lines.count(), 1);
		S_EQUAL(lines[0], QString("1\t865654\t865654\tC\tT\t0.000116"));

		//find 3
		lines = index.lines("chr1", 865655, 865663);
		I_EQUAL(lines.count(), 0);

		//find 4
		lines = index.lines("chr17", 865654, 865663);
		I_EQUAL(lines.count(), 0);
	}


	void lines2()
	{
		ChromosomalFileIndex index;
		index.load(TESTDATA("data_in/ChromosomalFileIndex.tsv"));

		VariantList data;
		data.load(TESTDATA("data_in/ChromosomalFileIndex.tsv"));

		for(int i=0; i<data.count(); ++i)
		{
			const Variant& v = data[i];
			QStringList lines = index.lines(v.chr().str(), v.start(), v.end());

			if (v.end()==866515 || v.end()==876499 || v.end()==880474)
			{
				I_EQUAL(lines.count(), 2);
			}
			else if (v.end()==156645197 || v.end()==156867776)
			{
				I_EQUAL(lines.count(), 3);
			}
			else if (v.end()==156570521)
			{
				I_EQUAL(lines.count(), 4);
			}
			else if (v.end()==156411782)
			{
				I_EQUAL(lines.count(), 6);
			}
			else
			{
				I_EQUAL(lines.count(), 1);
			}
		}
	}

	void isUpToDate()
	{
		IS_TRUE(!ChromosomalFileIndex::isUpToDate(TESTDATA("data_in/ChromosomalFileIndex.tsv")));
	}


	void create()
	{
		QFile::remove("out/ChromosomalFileIndex.tsv");
		QFile::remove("out/ChromosomalFileIndex.tsv.cidx");
		QFile::copy(TESTDATA("data_in/ChromosomalFileIndex.tsv"), "out/ChromosomalFileIndex.tsv");

		ChromosomalFileIndex index;
		index.create("out/ChromosomalFileIndex.tsv", 0, 1, 2, '#', 10);
		IS_TRUE(ChromosomalFileIndex::isUpToDate("out/ChromosomalFileIndex.tsv"));

		//find 1
		QPair<long, long> pos = index.filePosition("chr1", 865650, 865664);
		I_EQUAL(pos.first, 274);
		I_EQUAL(pos.second, 565);
		QStringList lines = index.lines("chr1", 865650, 865664);
		I_EQUAL(lines.count(), 2);
		S_EQUAL(lines[0], QString("1\t865654\t865654\tC\tT\t0.000116"));
		S_EQUAL(lines[1], QString("1\t865664\t865664\tC\tT\t0.000116"));

		//find 2
		lines = index.lines("chr1", 865654, 865663);
		I_EQUAL(lines.count(), 1);
		S_EQUAL(lines[0], QString("1\t865654\t865654\tC\tT\t0.000116"));

		//find 3
		lines = index.lines("chr1", 865655, 865663);
		I_EQUAL(lines.count(), 0);

		//find 4
		lines = index.lines("chr17", 865654, 865663);
		I_EQUAL(lines.count(), 0);

		//find 5
		lines = index.lines("chrX", 1, 2);
		I_EQUAL(lines.count(), 1);

		//find 5
		lines = index.lines("chrY", 1, 40);
		I_EQUAL(lines.count(), 2);

		//find 6
		lines = index.lines("chrM", 12, 40);
		I_EQUAL(lines.count(), 0);

		//find 7
		lines = index.lines("chrUN1", 2, 40);
		I_EQUAL(lines.count(), 1);
	}


	void regressionFirstMissing()
	{
		ChromosomalFileIndex index;

		index.load(TESTDATA("data_in/ChromosomalFileIndex2.gff"));

		//find 1
		QStringList lines = index.lines("1", 0, 1000000);
		I_EQUAL(lines.count(), 5);
		S_EQUAL(lines[0], QString("chr1	hgmd	variant_phenotype	949696	949696	.	+	.	ID=230352;accession=CI128669;alt=CG;aminoacid_change=N/A;citation_type=Primary;codon_change=N/A;codon_number=113;comments=N/A;confidence=High;disease=Mycobacterial disease%2C mendelian susceptibility to;ensembl=ENSG00000187608;entrez=9636;genomic_sequence=GTGGCCCACCTGAAGCAGCAAGTGAGCGGG(-/g)CTGGAGGGTGTGCAGGACGACCTGTTCTGG;hgmdAcc=CI128669;hgnc=ISG15;hgvs=NM_005101.3: c.339dupG%3B NP_005092.1: p.339dup;lsdb_source=N/A;mutationType=I;nucleotideChange=339dupG;omim=147571;pmid=22859821;pmid_notes=N/A;ref=C;rsid=N/A;uniprot=P05161;variantType=DM;feature=Mycobacterial disease  mendelian susceptibility to:339dupG;hyperlink=https://genometrax.biobase-international.com/static/hgmd_reports/CI128669"));
		S_EQUAL(lines[1], QString("chr1	hgmd	variant_phenotype	949739	949739	.	+	.	ID=291656;accession=CM128668;alt=T;aminoacid_change=E>*;citation_type=Primary;codon_change=cGAG-TAG;codon_number=127;comments=N/A;confidence=High;disease=Mycobacterial disease%2C mendelian susceptibility to;ensembl=ENSG00000187608;entrez=9636;genomic_sequence=GTGCAGGACGACCTGTTCTGGCTGACCTTC(G/T)AGGGGAAGCCCCTGGAGGACCAGCTCCCGC;hgmdAcc=CM128668;hgnc=ISG15;hgvs=NM_005101.3: c.379G>T%3B NP_005092.1: p.E127*;lsdb_source=N/A;mutationType=M;nucleotideChange=379G>T;omim=147571;pmid=22859821;pmid_notes=N/A;ref=G;rsid=N/A;uniprot=P05161;variantType=DM;feature=Mycobacterial disease  mendelian susceptibility to:379G>T;hyperlink=https://genometrax.biobase-international.com/static/hgmd_reports/CM128668"));
		S_EQUAL(lines[2], QString("chr1	hgmd	variant_phenotype	976962	976962	.	+	.	ID=290406;accession=CM126385;alt=T;aminoacid_change=Q>*;citation_type=Primary;codon_change=gCAG-TAG;codon_number=353;comments=N/A;confidence=High;disease=Congenital myasthenic syndrome;ensembl=ENSG00000188157;entrez=375790;genomic_sequence=CTCCTACGGCCCGAGAGCTGCCCTGCCCGG(C/T)AGGCGCCAGTGTGTGGGGACGACGGAGTCA;hgmdAcc=CM126385;hgnc=AGRN;hgvs=NM_198576.3: c.1057C>T%3B NP_940978.2: p.Q353*;lsdb_source=N/A;mutationType=M;nucleotideChange=1057C>T;omim=103320;pmid=22205389;pmid_notes=N/A;ref=C;rsid=N/A;uniprot=O00468;variantType=DM;feature=Congenital myasthenic syndrome:1057C>T;hyperlink=https://genometrax.biobase-international.com/static/hgmd_reports/CM126385"));
		S_EQUAL(lines[3], QString("chr1	hgmd	variant_phenotype	985955	985955	.	+	.	ID=270885;accession=CM094737;alt=C;aminoacid_change=G>R;citation_type=Primary;codon_change=gGGG-CGG;codon_number=1709;comments=N/A;confidence=High;disease=Myasthenia;ensembl=ENSG00000188157;entrez=375790;genomic_sequence=CGCCTGGAGTTCCGCTACGACCTGGGCAAG(G/C)GGGCAGCGGTCATCAGgtgggccggcaagg;hgmdAcc=CM094737;hgnc=AGRN;hgvs=NM_198576.3: c.5125G>C%3B NP_940978.2: p.G1709R;lsdb_source=N/A;mutationType=M;nucleotideChange=5125G>C;omim=103320;pmid=19631309;pmid_notes=N/A;ref=G;rsid=rs199476396;uniprot=O00468;variantType=DM;feature=Myasthenia:5125G>C;hyperlink=https://genometrax.biobase-international.com/static/hgmd_reports/CM094737"));
		S_EQUAL(lines[4], QString("chr1	hgmd	variant_phenotype	986143	986143	.	+	.	ID=290407;accession=CM126386;alt=T;aminoacid_change=V>F;citation_type=Primary;codon_change=gGTC-TTC;codon_number=1727;comments=N/A;confidence=High;disease=Congenital myasthenic syndrome;ensembl=ENSG00000188157;entrez=375790;genomic_sequence=GAGCCAGTCACCCTGGGAGCCTGGACCAGG(G/T)TCTCACTGGAGCGAAACGGCCGCAAGGGTG;hgmdAcc=CM126386;hgnc=AGRN;hgvs=NM_198576.3: c.5179G>T%3B NP_940978.2: p.V1727F;lsdb_source=N/A;mutationType=M;nucleotideChange=5179G>T;omim=103320;pmid=22205389;pmid_notes=N/A;ref=G;rsid=N/A;uniprot=O00468;variantType=DM;feature=Congenital myasthenic syndrome:5179G>T;hyperlink=https://genometrax.biobase-international.com/static/hgmd_reports/CM126386"));
	}

};
