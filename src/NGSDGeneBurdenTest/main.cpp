#include "ToolBase.h"
#include "BedFile.h"
#include "GeneBurdenTest.h"


class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Performs gene-wise Burden test on two sets of processed samples based on imported variants in the NGSD.");
		addInfile("cases", "Text file containing case sample (one processed sample per line)", false, true);
		addInfile("controls", "Text file containing case sample (one processed sample per line)", false, true);
		addInfile("genes", "Text file containing genes to test (one gene per line)", false, true);
		addOutfile("out", "Output TSV file containing the result of the Burden test.", false, true);

		//optional
		addInt("max_ngsd_count", "Maximum NGSD count of a variant to still be included.", true, 20);
		addFloat("max_gnomad_af", "Maximum gnomAD allele frequency (in %) of a variant to still be included.", true, 0.1);
		addString("impacts", "Comma separated list of impacts which should be included (allowed values: HIGH, MODERATE, LOW, MODIFIER)", true, "HIGH,MODERATE");
		addString("inheritance", "Inheritance mode to use. (allowed values: dominant, de-novo, recessive)", true, "dominant");
		addFlag("include_mosaic", "Include mosaic variants.");
		addFlag("predict_pathogenic", "add variants with moderate/low/modifier impact only if CADD >= 20 or SpliceAI >= 0.5.");
		addFlag("include_cnvs", "Include CNVs to test.");
		addFlag("ccr_only", "Limit test to constrained coding regions.");
		addInt("splice_region_size", "Extend coding region by this amount of bases.", true, 20);
		addInfile("excluded_regions", "BED file containing regions which should be excluded from the test.", true);
		addInt("threads", "Number of threads used to perform the test.", true, 4);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Activate debug output.");
		addFlag("skip_errors", "Only report errors, do not fail execution.");

		changeLog(2026, 6,  1, "Added live impact annotation.");
		changeLog(2026, 5, 22, "Added multithreading.");
		changeLog(2026, 5, 20, "Initial commit.");
	}

	virtual void main()
	{
		//read parameters
		BurdenTestParameters parameters;
		parameters.max_ngsd_count = getInt("max_ngsd_count");
		parameters.max_gnomad_af = getFloat("max_gnomad_af")/100.0;

		for (const QString& impact : getString("impacts").split(","))
		{
			if ((impact == "HIGH") || (impact == "MODERATE") || (impact == "LOW") || (impact == "MODIFIER"))
			{
				parameters.impacts << stringToVariantImpact(impact);
			}
			else
			{
				THROW(CommandLineParsingException, "Invalid impact '" + impact + "' provided!")
			}
		}

		if (getString("inheritance") == "dominant") parameters.inheritance = Inheritance::dominant;
		else if (getString("inheritance") == "de-novo") parameters.inheritance = Inheritance::de_novo;
		else if (getString("inheritance") == "recessive") parameters.inheritance = Inheritance::recessive;
		else THROW(CommandLineParsingException, "Invalid inheritance mode '" + getString("inheritance") + "' provided!")

		parameters.include_mosaic = getFlag("include_mosaic");
		parameters.predict_pathogenic = getFlag("predict_pathogenic");
		parameters.include_cnvs = getFlag("include_cnvs");
		parameters.ccr_only = getFlag("ccr_only");
		parameters.splice_region_size = getInt("splice_region_size");
		if (!getInfile("excluded_regions").isEmpty()) parameters.excluded_regions.load(getInfile("excluded_regions"));


		//read input files
		QStringList cases = Helper::loadTextFile(getInfile("cases"), true, '#', true);
		QStringList controls = Helper::loadTextFile(getInfile("controls"), true, '#', true);
		GeneSet genes = GeneSet::createFromFile(getInfile("genes"));


		//convert cases/controls to ps_ids
		NGSD db(getFlag("test"));
		QSet<int> ps_ids_cases;
		for (const QString& line : std::as_const(cases))
		{
			ps_ids_cases << db.processedSampleId(line.split('\t').at(0)).toInt();
		}
		QSet<int> ps_ids_controls;
		for (const QString& line : std::as_const(controls))
		{
			ps_ids_controls << db.processedSampleId(line.split('\t').at(0)).toInt();
		}

		//perform burden test
		GeneBurdenTest burden_test(ps_ids_cases, ps_ids_controls, genes, parameters, getInt("threads"), getFlag("test"), getFlag("debug"), getFlag("skip_errors"));
		QList<BurdenTestResult> results = burden_test.run_burden_test();

		//open output file
		QSharedPointer<QFile> out =Helper::openFileForWriting(getOutfile("out"), false, false);
		out->write("##cases=" + cases.join(",").toUtf8() + "\n");
		out->write("##controls=" + controls.join(",").toUtf8() + "\n");
		out->write("##genes=" + genes.join(",") + "\n");
		out->write("##" + parameters.toText().join("\n##") + "\n");

		//write results to file
		QByteArrayList headers;
		headers << "gene" << "p-value" << "n_hits_cases" << "hits_cases" << "n_hits_controls" << "hits_controls";
		if (parameters.include_cnvs) headers << "n_hits_cases_cnvs" << "hits_cases_cnvs" << "n_hits_controls_cnvs" << "hits_controls_cnvs";
		out->write("#" + headers.join("\t") + "\n");

		for (const BurdenTestResult& gene_result : std::as_const(results))
		{
			QByteArrayList line;
			line << gene_result.gene;
			line << QByteArray::number(gene_result.p_value);
			line << QByteArray::number(gene_result.hits_cases.size());

			QByteArrayList hits;
			for (auto iter = gene_result.hits_cases.constBegin(); iter != gene_result.hits_cases.constEnd(); ++iter)
			{
				hits << iter.key() + ": " + iter.value();
			}
			std::sort(hits.begin(), hits.end());
			line << hits.join("; ");

			line << QByteArray::number(gene_result.hits_controls.size());

			hits.clear();
			for (auto iter = gene_result.hits_controls.constBegin(); iter != gene_result.hits_controls.constEnd(); ++iter)
			{
				hits << iter.key() + ": " + iter.value();
			}
			std::sort(hits.begin(), hits.end());
			line << hits.join("; ");

			if (parameters.include_cnvs)
			{
				line << QByteArray::number(gene_result.hits_cases_cnv.size());

				hits.clear();
				for (auto iter = gene_result.hits_cases_cnv.constBegin(); iter != gene_result.hits_cases_cnv.constEnd(); ++iter)
				{
					hits << iter.key() + ": " + iter.value();
				}
				std::sort(hits.begin(), hits.end());
				line << hits.join("; ");

				line << QByteArray::number(gene_result.hits_controls_cnv.size());

				hits.clear();
				for (auto iter = gene_result.hits_controls_cnv.constBegin(); iter != gene_result.hits_controls_cnv.constEnd(); ++iter)
				{
					hits << iter.key() + ": " + iter.value();
				}
				std::sort(hits.begin(), hits.end());
				line << hits.join("; ");
			}

			out->write(line.join("\t") + "\n");
		}

		qDebug() << "finished!";

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
