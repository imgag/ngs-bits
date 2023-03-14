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


		QByteArray change_string = co_sp_anno.consequence.hgvs_c + ", " + co_sp_anno.consequence.hgvs_p + "\n\\line\n" + co_sp_anno.trans.nameWithVersion();
		QByteArray type_string = co_sp_anno.consequence.typesToString();


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
				row.addCell(cfdna_width, serum + " (" + QByteArray::number(alt_count) + "/" + QByteArray::number(depth) + ")\n\\line\n" + QByteArray::number(cfdna_entry.p_value, 'f', 2), RtfParagraph().setHorizontalAlignment("c").setFontSize(16));
			}
		}
		table.addRow(row);
	}

	table.setUniqueBorder(1, "brdrhair", 2);
	table.sortByCol(0);

	RtfTableRow header = RtfTableRow({"Gen", "Veränderung", "Typ", "Anteil Tumor"},{821,1900,1300,700}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
	for(int i=cfdna_idx_start; i<cfdna_idx_end; i++)
	{
		header.addCell(cfdna_width, "Anteil Plasma\n\\line\n" + data_.table.cfdna_samples[i].name.toUtf8() + "\n\\line\n(" + data_.table.cfdna_samples[i].date.toString("dd.MM.yyyy").toUtf8() + ")", RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
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
	out += bold("Anteil Tumor:") + " Allelfrequenze der gelisteten Variante (SNV, INDEL) in der Tumorprobe " + data_.table.tumor_sample.name + ", ";
	out += bold("Anteil Plasma:") + " Anteil der Allele mit der gelisteten Variante (SNV, INDEL) in den untersuchten Proben. In Klammern die Anzahl der Reads mit Variante / gesamte Anzahl der Reads und in der zweiten Zeile der p-Wert, ";
	out += bold("n.d.") + " nicht detektiert, " + bold("n/a:") + " nicht analysiert.\n\\line\n";

	return RtfParagraph(out).setFontSize(16).setHorizontalAlignment("j");
}


RtfTable SomaticcfDnaReport::partGeneralGeneticTable()
{
	RtfTable table;

	int cfdna_count = data_.table.cfdna_samples.count();
	int first_column_width = 2000;
	int cfdna_width = (9921 - first_column_width) / cfdna_count;

	QList<double> depth; // QC
	QList<double> d_depth; // QC ....71
	QList<double> error; // QC ....86

	//get QC values:
	foreach(auto sample, data_.table.cfdna_samples)
	{
		QCCollection qc = db_.getQCData(sample.ps_id);
		d_depth << qc.value("QC:2000071", true).asString().toDouble(); //two fold duplication
		error << qc.value("QC:2000086", true).asString().toDouble(); //two fold depth
		depth << qc.value("QC:2000025", true).asString().toDouble(); //depth

		ProcessedSampleData ps_data = db_.getProcessedSampleData(sample.ps_id);
	}

	table.addRow(RtfTableRow("Allgemeine genetische Charakteristika", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );
	RtfTableRow header = RtfTableRow("", first_column_width, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
	for(int i=0; i<cfdna_count; i++)
	{
		header.addCell(cfdna_width, data_.table.cfdna_samples[i].name.toUtf8(), RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c"));
	}
	table.addRow(header.setHeader().setBorders(1, "brdrhair", 2));

	RtfTableRow row_p_val;
	row_p_val.addCell(first_column_width, "Tumornachweis:");
	for(int i=0; i<data_.table.cfdna_samples.count(); i++)
	{
		QByteArray str_p_value = getMrdTableValue("MRD p-value", i);
		double p_value = str_p_value.toDouble();
		QByteArray p_value_final = p_value < 0.01 ? "<0.01" : QByteArray::number(p_value, 'f', 2);
		if (p_value < 0.05)
		{
			row_p_val.addCell(cfdna_width, "ja (p=" + p_value_final + ")");
		}
		else
		{
			row_p_val.addCell(cfdna_width, "nein (p=" + p_value_final + ")");
		}
	}
	table.addRow(row_p_val);

	RtfTableRow row_depth;
	row_depth.addCell(first_column_width, "Durchschnittliche Tiefe:");
	foreach(auto d, depth)
	{
		row_depth.addCell(cfdna_width, QByteArray::number(d, 'f', 0));
	}
	table.addRow(row_depth);

	RtfTableRow row_m_depth;
	row_m_depth.addCell(first_column_width, "Durchschnittliche Tiefe:\n\\line\n" + RtfText("(min. 1 Duplikat)").setFontSize(14).RtfCode());
	foreach(auto d, d_depth)
	{
		row_m_depth.addCell(cfdna_width, QByteArray::number(d, 'f', 0));
	}
	table.addRow(row_m_depth);

	RtfTableRow row_error;
	row_error.addCell(first_column_width, "Fehlerrate:\n\\line\n" + RtfText("(min. 1 Duplikat)").setFontSize(14).RtfCode());
	foreach(auto d, error)
	{
		QByteArrayList parts = QByteArray::number(d, 'e', 2).split('e');
		row_error.addCell(cfdna_width, parts[0] + "x10{\\super " + QByteArray::number(parts[1].toInt()) +"}" );
	}
	table.addRow(row_error);



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

	table.addRow( RtfTableRow( {"Proben-ID (Plasma): ", ids.join(", ")}, {2000,7921}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Tumor-ID: ", data_.table.tumor_sample.name.toUtf8()}, {2000,7921}, RtfParagraph().setFontSize(14)) );

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
	TranscriptList transcripts  = db_.transcriptsOverlapping(variant.chr(), variant.start(), variant.end(), 5000);
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
	anno.trans = transcripts[0];
	anno.consequence = hgvs_annotator.annotate(transcripts[0], variant);

	return anno;
}


RtfSourceCode SomaticcfDnaReport::formatDigits(double in, int digits)
{
	if(!BasicStatistics::isValidFloat(in)) return "n/a";

	return QByteArray::number(in, 'f', digits);
}

