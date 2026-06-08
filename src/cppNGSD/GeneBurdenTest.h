#ifndef GENEBURDENTEST_H
#define GENEBURDENTEST_H

#include "cppNGSD_global.h"
#include "NGSD.h"

enum class Inheritance
{
	dominant,
	de_novo,
	recessive
};

///Helper structs for burden test
struct CPPNGSDSHARED_EXPORT BurdenTestResult
{
	QByteArray gene;
	double p_value;
	QMap<QByteArray,QByteArray> hits_cases;
	QMap<QByteArray,QByteArray> hits_controls;
	//optional
	QMap<QByteArray,QByteArray> hits_cases_cnv;
	QMap<QByteArray,QByteArray> hits_controls_cnv;

	//for error handling in worker class
	QString error;
	QString warning;

	bool operator<(const BurdenTestResult other) const
	{
		return p_value < other.p_value;
	}

};

struct CPPNGSDSHARED_EXPORT BurdenTestParameters
{
	int max_ngsd_count;
	double max_gnomad_af;
	QSet<VariantImpact> impacts;
	Inheritance inheritance;
	bool include_mosaic;
	bool predict_pathogenic;
	bool include_cnvs;
	bool ccr_only;
	int splice_region_size;
	BedFile excluded_regions;

	QByteArrayList toText()
	{
		QByteArrayList text;
		text << "max_ngsd_count=" + QByteArray::number(max_ngsd_count);
		text << "max_gnomad_af=" + QByteArray::number(max_gnomad_af);
		QByteArrayList impact_str;
		for (VariantImpact impact : impacts)
		{
			impact_str << variantImpactToString(impact);
		}
		text <<  "impacts=" + impact_str.join(",");
		if (inheritance == Inheritance::dominant) text << "inheritance=dominant";
		else if (inheritance == Inheritance::de_novo) text << "inheritance=de-novo";
		else if (inheritance == Inheritance::recessive) text << "inheritance=recessive";
		text << "include_mosaic=" + QByteArray((include_mosaic)?"1":"0");
		text << "predict_pathogenic=" + QByteArray((predict_pathogenic)?"1":"0");
		text << "include_cnvs=" + QByteArray((include_cnvs)?"1":"0");
		text << "ccr_only=" + QByteArray((ccr_only)?"1":"0");
		text << "splice_region_size=" + QByteArray::number(splice_region_size);
		if (!excluded_regions.isEmpty()) text << "excluded_regions=" + excluded_regions.toText().replace("\t", "-").replace("\n", "; ").toUtf8();
		return text;
	};
};

class WorkerGeneBurdenTest : public QRunnable
{
public:
	WorkerGeneBurdenTest(BurdenTestResult& gene_result, const BurdenTestParameters& parameters, const QMap<QByteArray,BedFile>& ccr80_region, const QSet<int>& ps_ids_cases, const QSet<int>& ps_ids_controls,
						 const QByteArrayList& ps_ids, const QSet<int>& callset_ids_cases, const QSet<int>& callset_ids_controls, const BedFile& cnv_polymorphism_region, bool test, bool debug);
	virtual void run() override;

private:
	BurdenTestResult& gene_result_;
	const BurdenTestParameters& parameters_;
	const QMap<QByteArray,BedFile>& ccr80_region_;

	const QSet<int>& ps_ids_cases_;
	const QSet<int>& ps_ids_controls_;
	const QByteArrayList& ps_ids_;
	const QSet<int>& callset_ids_cases_;
	const QSet<int>& callset_ids_controls_;
	const BedFile& cnv_polymorphism_region_;

	// NGSD* db_;
	bool test_;
	bool debug_;

	QMap<int,Variant> getVariantsForRegion(const BedFile& regions);
	QString createGeneQuery(const BedFile& regions);
	QMap<QByteArray,QByteArray> getOccurences(const QSet<int>& variant_ids, const QSet<int>& ps_ids, const QMap<int, QSet<int> >& detected_variants, Inheritance inheritance);
	QMap<QByteArray,QByteArray> getOccurencesCNV(const QSet<int>& callset_ids, const BedFile& regions);
};



class CPPNGSDSHARED_EXPORT GeneBurdenTest
{

public:
	GeneBurdenTest(const QSet<int>& ps_ids_cases, const QSet<int>& ps_ids_controls, const GeneSet& genes, BurdenTestParameters parameters, int threads, bool test=false, bool debug=false, bool skip_errors=false);

	///Perform burden test
	QList<BurdenTestResult> run_burden_test();

private:
	QSet<int> ps_ids_cases_;
	QSet<int> ps_ids_controls_;
	GeneSet genes_;
	BurdenTestParameters parameters_;
	NGSD db_;
	bool test_;
	int threads_;
	bool debug_;
	bool skip_errors_;

	//helper to simplify SQL query generation
	QByteArrayList ps_ids_;

	//CNVs
	QSet<int> callset_ids_cases_;
	QSet<int> callset_ids_controls_;
	BedFile cnv_polymorphism_region_;

	//CCR80 region
	GeneSet ccr_genes_;
	QMap<QByteArray,BedFile> ccr80_region_;
	void initCCR();

};

#endif // GENEBURDENTEST_H
