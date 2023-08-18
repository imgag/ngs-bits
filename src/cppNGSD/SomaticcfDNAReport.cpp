#include <QDebug>
#include <cmath>
#include "NGSD.h"
#include "SomaticcfDNAReport.h"
#include "SomaticReportHelper.h"
#include "TSVFileStream.h"
#include "VariantHgvsAnnotator.h"
#include "VariantList.h"



SomaticcfDNAReportData::SomaticcfDNAReportData(const SomaticReportSettings& other, const CfdnaDiseaseCourseTable& table_data)
	: SomaticReportSettings(other)
	, table(table_data)
{

}

SomaticcfDnaReport::SomaticcfDnaReport(const SomaticcfDNAReportData& data)
	: db_()
	, data_(data)
{
}

void SomaticcfDnaReport::writeRtf(QByteArray out_file)
{
	doc_.setDefaultFontSize(16);

	doc_.addColor(191,191,191);
	doc_.addColor(161,161,161);
	doc_.addColor(255,255,0);
	doc_.addColor(242, 242, 242);
	doc_.addColor(255,0,0);

	doc_.addPart(partResultTable().RtfCode());
	doc_.addPart(RtfParagraph("*AF: Allelfrequenz, Anteil mutierte Fragmente").setFontSize(16).setHorizontalAlignment("j").RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());


	if (data_.table.cfdna_samples.count() <= 3)
	{
		doc_.addPart(RtfParagraph("Patientenspezifische somatische Variante(n):").setFontSize(18).setBold(true).RtfCode());
		doc_.addPart(partSnvTable(0, data_.table.cfdna_samples.count()).RtfCode());
		doc_.addPart(partSnvExplanation().RtfCode());
	}
	else
	{
		int i=1;
		while (i*3 < data_.table.cfdna_samples.count())
		{
			doc_.addPart(RtfParagraph("Patientenspezifische somatische Variante(n) - Teil " + QByteArray::number(i) + ":").setFontSize(18).setBold(true).RtfCode());
			doc_.addPart(partSnvTable((i-1)*3, (i*3)).RtfCode());
			doc_.addPart(partSnvExplanation().RtfCode());
			i++;
		}

		doc_.addPart(RtfParagraph("Patientenspezifische somatische Variante(n) - Teil " + QByteArray::number(i) + ":").setFontSize(18).setBold(true).RtfCode());
		doc_.addPart(partSnvTable((i-1)*3, data_.table.cfdna_samples.count()).RtfCode());
		doc_.addPart(partSnvExplanation().RtfCode());
	}
	doc_.addPart(RtfParagraph("").RtfCode());

	doc_.addPart(partGeneralGeneticTable().RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());

	doc_.addPart(RtfParagraph("Technischer Report").setFontSize(18).setBold(true).RtfCode());
	doc_.addPart(partGeneralInfo().RtfCode());

	doc_.save(out_file);
}

RtfTable SomaticcfDnaReport::partResultTable()
{
	RtfTable table;
	for(int i=0; i<data_.table.cfdna_samples.count(); i++)
	{
		RtfTableRow row;


		row.addCell(3321, data_.table.cfdna_samples[i].name.toUtf8(), RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		if (data_.table.cfdna_samples[i].sampling_date.isValid())
		{
			row.addCell(1650, data_.table.cfdna_samples[i].sampling_date.toString("dd.MM.yyyy").toUtf8(), RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		}
		else
		{
			row.addCell(1650, "", RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		}

		double maxAF = getMaxAf(i);
		if (maxAF > 0 && maxAF < 0.001)
		{
			row.addCell(1650, "< 0.001" , RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		}
		else
		{
			row.addCell(1650, formatDigits(maxAF, 3) , RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		}
		row.addCell(1650, getMeanAf(i), RtfParagraph().setHorizontalAlignment("c").setFontSize(16));

		QByteArray str_p_value = getMrdTableValue("MRD p-value", i);
		double p_value = str_p_value.toDouble();
		QByteArray p_value_final = p_value < 0.01 ? "<0.01" : QByteArray::number(p_value, 'f', 2);
		if (p_value < 0.05)
		{
			row.addCell(1650, "ja (p=" + p_value_final + ")", RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		}
		else
		{
			row.addCell(1650, "nein (p=" + p_value_final + ")", RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		}

		table.addRow(row);
	}

	table.setUniqueBorder(1, "brdrhair", 2);

	RtfTableRow header = RtfTableRow({"Probe", "Datum", "Max. AF*", "Mittelwert AF*", "Tumornachweis"},{3321,1650, 1650, 1650, 1650}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
	table.prependRow(header.setHeader().setBorders(1, "brdrhair", 2));

	table.prependRow(RtfTableRow("Probenübersicht", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	return table;

}


RtfTable SomaticcfDnaReport::partSnvTable(int cfdna_idx_start, int cfdna_idx_end)
{
	RtfTable table;
	if(cfdna_idx_end > data_.table.cfdna_samples.count())
	{
		cfdna_idx_end = data_.table.cfdna_samples.count();
	}

	int cfdna_count = cfdna_idx_end - cfdna_idx_start;
	int cfdna_width = 5200 / cfdna_count;
	for(int i=0; i<data_.table.lines.count(); i++)
	{
		RtfTableRow row;

		const auto& line = data_.table.lines.at(i);
		const VcfLine& variant = line.tumor_vcf_line;
		//skip ID SNPs
		if (variant.id().contains("ID")) continue;

		CodingSplicingAnno co_sp_anno = getPreferedCodingAndSplicing(variant);

		QByteArray change_string;
		QByteArray type_string;

		if (co_sp_anno.trans.isValid())
		{
			change_string = co_sp_anno.consequence.hgvs_c + ", " + co_sp_anno.consequence.hgvs_p + "\n\\line\n" + co_sp_anno.trans.nameWithVersion();
			type_string = co_sp_anno.consequence.typesToString();
		}
		else
		{
			change_string = variant.chr().strNormalized(false) + ":g." + QByteArray::number(variant.start()) + variant.ref() + ">" + variant.alt()[0];
			type_string = "intergenic";
		}


		QByteArray tumor_af = QByteArray::number(variant.info("tumor_af").toDouble(), 'f', 2);

		row.addCell(821, co_sp_anno.trans.gene(), RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		row.addCell(1900, change_string, RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		row.addCell(1300, cleanConsequenceString(type_string), RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
		row.addCell(700, tumor_af, RtfParagraph().setHorizontalAlignment("c").setFontSize(16));

		//fill table
		for(int c=cfdna_idx_start; c<cfdna_idx_end; c++)
		{
			auto cfdna_entry = line.cfdna_columns[c];
			double cfdna_af = cfdna_entry.multi_af;
			int alt_count = cfdna_entry.multi_alt;
			int depth = cfdna_entry.multi_ref + alt_count;


			if(std::isnan(cfdna_af))
			{
				//special handling: VCF not found mark yellow
				row.addCell(cfdna_width, "not found", RtfParagraph().highlight(3));
			}
			else
			{
				QByteArray serum;
				//default case

				if (cfdna_af == 0)
				{
					serum = QByteArray::number(0);
				}
				else if (cfdna_af < 0.001)
				{
					serum = "< 0.001";
				}
				else
				{
					serum = QByteArray::number(cfdna_af, 'f', 3);
				}
				row.addCell(cfdna_width, serum + " (" + QByteArray::number(alt_count) + "/" + QByteArray::number(depth) + ")", RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
			}
		}
		table.addRow(row);
	}

	table.setUniqueBorder(1, "brdrhair", 2);
	table.sortByCol(0);

	RtfTableRow header = RtfTableRow({"Gen", "Veränderung", "Typ", "Anteil Tumor"},{821,1900,1300,700}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
	for(int i=cfdna_idx_start; i<cfdna_idx_end; i++)
	{
		if(data_.table.cfdna_samples[i].sampling_date.isValid())
		{
			header.addCell(cfdna_width, "Anteil Plasma\n\\line\n" + data_.table.cfdna_samples[i].name.toUtf8() + "\n\\line\n(" + data_.table.cfdna_samples[i].sampling_date.toString("dd.MM.yyyy").toUtf8() + ")", RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
		}
		else
		{
			header.addCell(cfdna_width, "Anteil Plasma\n\\line\n" + data_.table.cfdna_samples[i].name.toUtf8() + "\n\\line\n()", RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
		}
	}
	table.prependRow(header.setHeader().setBorders(1, "brdrhair", 2));

	table.prependRow(RtfTableRow("Punktmutationen (SNVs) und kleine Insertionen/Deletionen (INDELs)", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	return table;
}

RtfParagraph SomaticcfDnaReport::partSnvExplanation()
{
	auto bold = [](RtfSourceCode text)
	{
		return RtfText(text).setBold(true).setFontSize(16).RtfCode();
	};

	RtfSourceCode out = "";

	out += bold("COV:");
	out += " Anzahl der Reads an der untersuchten genomischen Position, ";
	out += bold("SNV:");
	out += " Punktmutationen (Single Nucleotide Variant), ";
	out += bold("INDEL:");
	out += " Insertionen/Deletionen, ";
	out += bold("Veränderung:") + " Kodierende Position und Auswirkung auf das Protein, ";
    out += bold("Anteil Tumor:") + " Allelfrequenze der gelisteten Variante (SNV, INDEL) in der Tumorprobe " + data_.table.tumor_sample.name.toUtf8() + ", ";
	out += bold("Anteil Plasma:") + " Anteil der Allele mit der gelisteten Variante (SNV, INDEL) in den untersuchten Proben. In Klammern die Anzahl der Reads mit mind. einem Duplikat mit Variante / gesamte Anzahl der Reads mit mind. einem Duplikat, ";
	out += bold("n.d.") + " nicht detektiert, " + bold("n/a:") + " nicht analysiert.\n\\line\n";

	return RtfParagraph(out).setFontSize(16).setHorizontalAlignment("j");
}


RtfTable SomaticcfDnaReport::partGeneralGeneticTable()
{
	RtfTable table;

//	int cfdna_count = data_.table.cfdna_samples.count();
//	int first_column_width = 2000;
//	int cfdna_width = (9921 - first_column_width) / cfdna_count;

//	QList<double> depth; // QC
//	QList<double> d_depth; // QC ....71
//	QList<double> error; // QC ....86

	//get QC values:
//	foreach(auto sample, data_.table.cfdna_samples)
//	{
//		QCCollection qc = db_.getQCData(sample.ps_id);
//		d_depth << qc.value("QC:2000071", true).asDouble(); //two fold duplication
//		error << qc.value("QC:2000086", true).asDouble(); //two fold depth
//		depth << qc.value("QC:2000025", true).asDouble(); //depth

//		ProcessedSampleData ps_data = db_.getProcessedSampleData(sample.ps_id);
//	}

	table.addRow(RtfTableRow("Qualitätsparameter", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );
	RtfTableRow header = RtfTableRow({"Probe", "Durchschnittliche Tiefe:", "Durchschnittliche Tiefe:\n\\line\n" + RtfText("(min. 1 Duplikat)").setFontSize(14).RtfCode(), "Fehlerrate:\n\\line\n" + RtfText("(min. 1 Duplikat)").setFontSize(14).RtfCode()},{2121,2600,2600,2600}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
	table.addRow(header.setHeader().setBorders(1, "brdrhair", 2));

	foreach(auto sample, data_.table.cfdna_samples)
	{

		QCCollection qc = db_.getQCData(sample.ps_id);
		double d_depth = qc.value("QC:2000071", true).asDouble(); //two fold duplication
		double error = qc.value("QC:2000086", true).asDouble(); //two fold depth
		double depth = qc.value("QC:2000025", true).asDouble(); //depth


		QByteArrayList parts = QByteArray::number(error, 'e', 2).split('e');
		QByteArray error_str = parts[0] + "x10{\\super " + QByteArray::number(parts[1].toInt()) +"}";

		ProcessedSampleData ps_data = db_.getProcessedSampleData(sample.ps_id);



		RtfTableRow sample_row = RtfTableRow({sample.name.toUtf8(), formatDigits(depth, 0), formatDigits(d_depth, 0), error_str},{2121,2600,2600,2600}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
		table.addRow(sample_row);
	}

	table.setUniqueBorder(1, "brdrhair", 2);

	return table;
}

QByteArray SomaticcfDnaReport::getMrdTableValue(const QByteArray& type, int cfdna_idx)
{
	int idx;
	QStringList data;

	if (type == "MRD log10")
	{
		idx = data_.table.mrd_tables[cfdna_idx].columnIndex("MRD_log10");
		data = data_.table.mrd_tables[cfdna_idx].extractColumn(idx);
	}
	else if (type == "MRD p-value")
	{
		idx = data_.table.mrd_tables[cfdna_idx].columnIndex("MRD_pval");
		data = data_.table.mrd_tables[cfdna_idx].extractColumn(idx);
	}
	else if (type == "Depth")
	{
		idx = data_.table.mrd_tables[cfdna_idx].columnIndex("SUM_DP");
		data = data_.table.mrd_tables[cfdna_idx].extractColumn(idx);
	}
	else if (type == "Alt")
	{
		idx = data_.table.mrd_tables[cfdna_idx].columnIndex("SUM_ALT");
		data = data_.table.mrd_tables[cfdna_idx].extractColumn(idx);
	}
	else if (type == "Mean AF")
	{
		idx = data_.table.mrd_tables[cfdna_idx].columnIndex("Mean_AF");
		data = data_.table.mrd_tables[cfdna_idx].extractColumn(idx);
	}
	else if (type == "Median AF")
	{
		idx = data_.table.mrd_tables[cfdna_idx].columnIndex("Median_AF");
		data = data_.table.mrd_tables[cfdna_idx].extractColumn(idx);
	}
	else
	{
		THROW(ProgrammingException, "Unknown type when getting MRD table value. This should not happen. Please inform the bioinformatics team.");
	}

	if (data.count() != 1)
	{
		THROW(ArgumentException, "Tsv file with MRD values for cfDNA " + data_.table.cfdna_samples[cfdna_idx].name + " has mutiple lines. Only one expected!");
	}

	return data[0].toUtf8();
}


RtfTable SomaticcfDnaReport::partGeneralInfo()
{
	RtfTable table;

//	table.addRow( RtfTableRow("Allgemeine Informationen", 9921, RtfParagraph().setFontSize(18).setBold(true)).setHeader() );

	QByteArrayList ids;
	QSet<QString> sys;
	foreach(auto sample, data_.table.cfdna_samples)
	{
		ids.append(sample.name.toUtf8());
		ProcessedSampleData ps_data = db_.getProcessedSampleData(sample.ps_id);
		sys << ps_data.processing_system;
	}

	table.addRow( RtfTableRow( {"Tumor-ID: ", data_.table.tumor_sample.name.toUtf8()}, {2000,7921}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Plasma-ID(s): ", ids.join(", ")}, {2000,7921}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Prozessierungssystem:", "Patientenspezifisches Panel " + sys.toList().join(", ").toUtf8()}, {2000,7921}, RtfParagraph().setFontSize(14)) );

	return table;
}

QByteArray SomaticcfDnaReport::cleanConsequenceString(QByteArray consequence)
{
	consequence = consequence.replace("_variant", "");
	consequence = consequence.replace("coding_sequence&", "");
	consequence = consequence.replace("&protein_altering", "");
	consequence = consequence.replace("&", ", ");

	return consequence;
}


CodingSplicingAnno SomaticcfDnaReport::getPreferedCodingAndSplicing(const VcfLine& variant)
{
	//update gene and coding_and_splicing column with live annotation

	//get all transcripts containing the variant
	TranscriptList transcripts = db_.transcriptsOverlapping(variant.chr(), variant.start(), variant.end());

	// if there are no transcripts that are directly affected, search for transcripts up- and downstream
	if (transcripts.count() == 0)
	{
		transcripts = db_.transcriptsOverlapping(variant.chr(), variant.start(), variant.end(), 5000);
	}

	transcripts.sortByRelevance();

	//find prefered transcript and annotate:
	FastaFileIndex genome_idx(Settings::string("reference_genome"));
	VariantHgvsAnnotator hgvs_annotator(genome_idx);

	foreach(const Transcript& trans, transcripts)
	{
		if (trans.isPreferredTranscript())
		{
			CodingSplicingAnno anno;
			anno.trans = trans;
			anno.consequence = hgvs_annotator.annotate(trans, variant);;
			return anno;
		}
	}

	CodingSplicingAnno anno;
	if (transcripts.count() >= 1)
	{
		anno.trans = transcripts[0];
		anno.consequence = hgvs_annotator.annotate(transcripts[0], variant);
	}

	return anno;
}

double SomaticcfDnaReport::getMaxAf(int cfdna_idx)
{
	double max = -1;

	for(int i=0; i<data_.table.lines.count(); i++)
	{
		auto line = data_.table.lines[i];
		const VcfLine& variant = line.tumor_vcf_line;
		//skip ID SNPs
		if (variant.id().contains("ID")) continue;
		auto cfdna_entry = line.cfdna_columns[cfdna_idx];
		double var_af = cfdna_entry.multi_af;

		if (var_af > max) max = var_af;
	}

	return max;
}

QByteArray SomaticcfDnaReport::getMeanAf(int cfdna_idx)
{
	double sum = 0;
	double count = 0;
	for(int i=0; i<data_.table.lines.count(); i++)
	{
		auto line = data_.table.lines[i];
		const VcfLine& variant = line.tumor_vcf_line;
		//skip ID SNPs
		if (variant.id().contains("ID")) continue;
		auto cfdna_entry = line.cfdna_columns[cfdna_idx];
		sum += cfdna_entry.multi_af;
		count++;
	}
	return formatDigits(sum/count, 3);
}

RtfSourceCode SomaticcfDnaReport::formatDigits(double in, int digits)
{
	if(!BasicStatistics::isValidFloat(in)) return "n/a";

	return QByteArray::number(in, 'f', digits);
}

