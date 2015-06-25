#include "../TestFramework.h"
#include "VariantFilter.h"
#include "VariantList.h"

class VariantFilter_Test
		: public QObject
{
	Q_OBJECT

private slots:

	void pass()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));

		/*
	#chr	start	end	ref	obs	genotype	snp_q	depth	map_q	variant	variant_frequency	region	gene	variant_details	coding	snp_1000g2012feb_all	snp_snp137	ljb_phylop	ljb_mt	esp6500si_ea	cosmic61	repeatmasker	omim_ids	omim_titles	omim_disorders	quality_filter	ihdb_hom_13	ihdb_het_13	ihdb_wt_13	ihdb_allsys	vus	validated
	chr1	155205047	155205047	C	T	het	225	728	37	SNV	0.5084	exonic	GBA	nonsynonymous SNV	GBA:NM_001171812:exon9:c.G1297A:p.D433N,GBA:NM_001005741:exon11:c.G1444A:p.D482N,GBA:NM_001005742:exon11:c.G1444A:p.D482N,GBA:NM_000157:exon10:c.G1444A:p.D482N,GBA:NM_001171811:exon9:c.G1183A:p.D395N,	0.0014	rs75671029	0.1614	0.9521	.	.	.	606463	Glucosidase, acid beta	Gaucher disease, type I, 230800(3); Gaucher disease, type II, 230900(3); Gaucher disease, type III, 231000 (3); Gaucher disease, type IIIC, 231005 (3); Gaucher disease, perinatal lethal, 608013 (3)	passed	0	0	1	0	.	.
	chr2	198362018	198362018	T	C	hom	222	538	37	SNV	1	exonic	HSPD1	synonymous SNV	HSPD1:NM_199440:exon3:c.A273G:p.K91K,HSPD1:NM_002156:exon3:c.A273G:p.K91K,	0.66	rs8539	.	.	0.6842	.	.	118190	Heat-shock 60kD protein 1	Spastic paraplegia-13, 605280 (3); Leukodystrophy, hypomyelinating,4, 612233 (3)	passed	0.2308	0.6923	0.0769	80	.	.
	chr2	202575821	202575821	G	A	hom	222	503	37	SNV	1	exonic	ALS2	synonymous SNV	ALS2:NM_020919:exon26:c.C4015T:p.L1339L,	0.91	rs3219168	.	.	0.9205	.	.	606352	Alsin	Amyotrophic lateral sclerosis, juvenile, 205100 (3); Primarylateral sclerosis, juvenile, 606353 (3); Spastic paralysis, infantile onset ascending, 607225 (3)	passed	0.6154	0.3846	0	81	.	.
	chr2	202598113	202598113	C	T	hom	222	780	37	SNV	0.9961	exonic	ALS2	synonymous SNV	ALS2:NM_020919:exon13:c.G2466A:p.V822V,	0.43	rs2276615	.	.	0.6033	.	.	606352	Alsin	Amyotrophic lateral sclerosis, juvenile, 205100 (3); Primarylateral sclerosis, juvenile, 606353 (3); Spastic paralysis, infantile onset ascending, 607225 (3)	passed	0.0769	0.3846	0.5385	64	.	.
	chr2	202625615	202625615	C	T	hom	222	982	37	SNV	1	exonic	ALS2	nonsynonymous SNV	ALS2:NM_001135745:exon4:c.G1102A:p.V368M,ALS2:NM_020919:exon4:c.G1102A:p.V368M,	0.90	rs3219156	0.0032	0.0000	0.8984	.	.	606352	Alsin	Amyotrophic lateral sclerosis, juvenile, 205100 (3); Primarylateral sclerosis, juvenile, 606353 (3); Spastic paralysis, infantile onset ascending, 607225 (3)	passed	0.3846	0.6154	0	82	.	.
	chr2	241680802	241680802	G	A	het	225	761	37	SNV	0.5106	intronic	KIF1A	.	.	0.28	rs56024577	.	.	0.2506	.	.				passed	0.0769	0.2308	0.6923	28	.	.
	chr2	241700676	241700676	G	A	het	225	405	37	SNV	0.5391	exonic	KIF1A	synonymous SNV	KIF1A:NM_004321:exon22:c.C2208T:p.A736A,KIF1A:NM_001244008:exon24:c.C2235T:p.A745A,	0.02	rs35945835	.	.	0.0523	.	.				passed	0	0.3846	0.6154	9	.	.
	chr2	241713646	241713646	A	G	het	225	657	37	SNV	0.5238	exonic	KIF1A	synonymous SNV	KIF1A:NM_004321:exon11:c.T991C:p.L331L,KIF1A:NM_001244008:exon12:c.T991C:p.L331L,	0.50	rs1063353	.	.	0.3466	.	.				passed	0.3077	0.5385	0.1538	37	.	.
	chr3	33055721	33055721	A	G	hom	222	1032	37	SNV	1	exonic	GLB1	nonsynonymous SNV	GLB1:NM_000404:exon15:c.T1561C:p.C521R,GLB1:NM_001079811:exon15:c.T1471C:p.C491R,GLB1:NM_001135602:exon12:c.T1168C:p.C390R,	0.94	rs4302331	0.1686	0.0000	0.9982	.	.	611458	Galactosidase, beta-1	GM1-gangliosidosis, type I, 230500 (3); GM1-gangliosidosis, type II,230600 (3); GM1-gangliosidosis, type III, 230650 (3); Mucopolysaccharidosis type IVB (Morquio), 253010 (3)	passed	1	0	0	137	.	.
	chr3	33138544	33138544	A	G	hom	222	28	37	SNV	1	exonic	GLB1	synonymous SNV	GLB1:NM_000404:exon1:c.T34C:p.L12L,GLB1:NM_001135602:exon1:c.T34C:p.L12L,	0.93	rs7614776	.	.	0.8855	.	.	611458	Galactosidase, beta-1	GM1-gangliosidosis, type I, 230500 (3); GM1-gangliosidosis, type II,230600 (3); GM1-gangliosidosis, type III, 230650 (3); Mucopolysaccharidosis type IVB (Morquio), 253010 (3)	passed	0.7692	0.1538	0.0769	138	.	.
	...
	chr19	46057081	46057081	A	G	hom	222	407	37	SNV	1	exonic	OPA3	synonymous SNV	OPA3:NM_025136:exon2:c.T231C:p.A77A,	0.77	rs3826860	.	.	0.6428	.	.	606580	OPA3 gene	3-methylglutaconic aciduria, type III, 258501 (3); Optic atrophyand cataract, 165300 (3)	passed	0.4615	0.4615	0.0769	118	.	.
	chr20	3870124	3870124	G	C	hom	222	854	37	SNV	1	exonic	PANK2	nonsynonymous SNV	PANK2:NM_153638:exon1:c.G377C:p.G126A,	0.88	rs3737084	0.9816	0.0000	0.9156	.	.	606157	Pantothenate kinase 2	Neurodegeneration with brain iron accumulation 1, 234200 (3);HARP syndrome, 607236 (3)	passed	0.6923	0.3077	0	50	.	.
	chr22	38528888	38528888	C	T	het	225	1132	37	SNV	0.4894	exonic	PLA2G6	nonsynonymous SNV	PLA2G6:NM_001004426:exon7:c.G1027A:p.A343T,PLA2G6:NM_003560:exon7:c.G1027A:p.A343T,PLA2G6:NM_001199562:exon7:c.G1027A:p.A343T,	0.01	rs11570680	0.9721	0.9649	0.0128	.	.	603604	Phospholipase A2, group VI	Infantile neuroaxonal dystrophy 1, 256600 (3); Neurodegenerationwith brain iron accumulation 2B, 610217 (3); Karak syndrome, 610217 (3); Parkinson disease 14, 612953 (3)	passed	0	0	1	14	.	.
	chrX	73641252	73641252	T	G	het	7.8	621	39	SNV	0.2406	upstream	SLC16A2	.	.	.	.	0.9299	0.9999	.	.	314	300095	Solute carrier family 16 (monocarboxylic acid transporters),	Allan-Herndon-Dudley syndrome, 300523 (3)	failed	0	0.0769	0.9231	1	.	.
	chrX	153005605	153005605	G	A	het	225	453	37	SNV	0.4424	exonic	ABCD1	synonymous SNV	ABCD1:NM_000033:exon6:c.G1548A:p.L516L,	0.07	rs41314153	.	.	0.1084	.	.	300371	ATP-binding cassette, subfamily D, member 1	Adrenoleukodystrophy, 300100 (3); Adrenomyeloneuropathy, 300100 (3)	passed	0.2308	0	0.7692	20	.	.
	chrX	153009197	153009197	G	C	het	225	701	37	SNV	0.5368	UTR3	ABCD1	.	.	0.70	rs2229539	.	.	0.6915	.	.	300371	ATP-binding cassette, subfamily D, member 1	Adrenoleukodystrophy, 300100 (3); Adrenomyeloneuropathy, 300100 (3)	passed	0.7692	0.2308	0	67	.	.
	*/

		VariantFilter filter("bla", "chr IS chr1");
		QCOMPARE(filter.pass(vl, 0), true);
		QCOMPARE(filter.pass(vl, 1), false);

		filter = VariantFilter("bla", "chr IS_NOT chr1 && start >= 202575822");
		QCOMPARE(filter.pass(vl, 0), false);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), false);
		QCOMPARE(filter.pass(vl, 3), true);
		QCOMPARE(filter.pass(vl, 4), true);


		filter = VariantFilter("bla", "chr CONTAINS X && start <= 153005605");
		QCOMPARE(filter.pass(vl, 0), false);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), false);
		QCOMPARE(filter.pass(vl, 72), true);
		QCOMPARE(filter.pass(vl, 73), true);
		QCOMPARE(filter.pass(vl, 74), false);

		filter = VariantFilter("bla", "chr CONTAINS X || start < 198362018");
		QCOMPARE(filter.pass(vl, 0), true);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), false);
		QCOMPARE(filter.pass(vl, 72), true);
		QCOMPARE(filter.pass(vl, 73), true);
		QCOMPARE(filter.pass(vl, 74), true);

		filter = VariantFilter("bla", "chr CONTAINS 1 || start > 198362018");
		QCOMPARE(filter.pass(vl, 0), true);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), true);
		QCOMPARE(filter.pass(vl, 72), false);
		QCOMPARE(filter.pass(vl, 73), false);
		QCOMPARE(filter.pass(vl, 74), false);

		filter = VariantFilter("bla", "chr CONTAINS_NOT 2 || start == 198362018");
		QCOMPARE(filter.pass(vl, 0), true);
		QCOMPARE(filter.pass(vl, 1), true);
		QCOMPARE(filter.pass(vl, 2), false);
		QCOMPARE(filter.pass(vl, 72), true);
		QCOMPARE(filter.pass(vl, 73), true);
		QCOMPARE(filter.pass(vl, 74), true);

		filter = VariantFilter("bla", "chr IS chr2 || start == 198362018");
		QCOMPARE(filter.pass(vl, 0), false);
		QCOMPARE(filter.pass(vl, 1), true);
		QCOMPARE(filter.pass(vl, 2), true);
		QCOMPARE(filter.pass(vl, 72), false);
		QCOMPARE(filter.pass(vl, 73), false);
		QCOMPARE(filter.pass(vl, 74), false);

		filter = VariantFilter("bla", "chr IS_NOT chr2 || start == 198362018");
		QCOMPARE(filter.pass(vl, 0), true);
		QCOMPARE(filter.pass(vl, 1), true);
		QCOMPARE(filter.pass(vl, 2), false);
		QCOMPARE(filter.pass(vl, 72), true);
		QCOMPARE(filter.pass(vl, 73), true);
		QCOMPARE(filter.pass(vl, 74), true);

		filter = VariantFilter("bla", "chr IS_NOT chr2 && start >= 155205047 && depth >= 500");
		QCOMPARE(filter.pass(vl, 0), true);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), false);
		QCOMPARE(filter.pass(vl, 72), false);
		QCOMPARE(filter.pass(vl, 73), false);
		QCOMPARE(filter.pass(vl, 74), false);

		filter = VariantFilter("bla", "chr IS chr2 && start > 198362018 || chr CONTAINS chrX && end < 153005605");
		QCOMPARE(filter.pass(vl, 0), false);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), true);
		QCOMPARE(filter.pass(vl, 72), true);
		QCOMPARE(filter.pass(vl, 73), false);
		QCOMPARE(filter.pass(vl, 74), false);

		filter = VariantFilter("bla", "chr IS chr2 && start > 198362018 || chr CONTAINS chrX && end < 153005605");
		QCOMPARE(filter.pass(vl, 0), false);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), true);
		QCOMPARE(filter.pass(vl, 72), true);
		QCOMPARE(filter.pass(vl, 73), false);
		QCOMPARE(filter.pass(vl, 74), false);

		filter = VariantFilter("bla", "ref IS G && obs IS A && genotype IS het");
		QCOMPARE(filter.pass(vl, 0), false);
		QCOMPARE(filter.pass(vl, 1), false);
		QCOMPARE(filter.pass(vl, 2), false);
		QCOMPARE(filter.pass(vl, 72), false);
		QCOMPARE(filter.pass(vl, 73), true);
		QCOMPARE(filter.pass(vl, 74), false);
	}

	void multiPass()
	{
		VariantList vl;
		vl.load(QFINDTESTDATA("data_in/panel.tsv"));

		QVector<VariantFilter> filters;
		filters.append(VariantFilter("bla", "*1000g* IS || *1000g* IS invalid || *1000g* < 0.01 || chr IS chrM"));
		filters.append(VariantFilter("bla", "*1000g* IS || *1000g* IS invalid || *1000g* < 0.05 || chr IS chrM"));
		filters.append(VariantFilter("bla", "*esp6500* IS || *1000g* IS invalid || *esp6500* < 0.01 || chr IS chrM"));
		filters.append(VariantFilter("bla", "region CONTAINS exonic || region CONTAINS splicing || chr IS chrM"));
		filters.append(VariantFilter("bla", "region CONTAINS_NOT ncRNA || chr IS chrM"));
		filters.append(VariantFilter("bla", "variant_details IS_NOT synonymous SNV || chr IS chrM"));
		filters.append(VariantFilter("bla", "snp_q >= 20 && map_q >= 20 && depth >= 10|| chr IS chrM"));
		filters.append(VariantFilter("bla", "ihdb_allsys <= 50|| chr IS chrM"));
		filters.append(VariantFilter("bla", "ihdb_allsys <= 5|| chr IS chrM"));
		filters.append(VariantFilter("bla", "classification IS  || classification > 2 || chr IS chrM"));
		filters.append(VariantFilter("bla", "chr IS_NOT chrM || variant_frequency < 0.98"));

		QBitArray pass = VariantFilter::multiPass(vl, filters);
		QCOMPARE(pass.count(true), 2);
	}

};

TFW_DECLARE(VariantFilter_Test)

