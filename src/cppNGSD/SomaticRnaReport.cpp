#include <QDebug>
#include "NGSD.h"
#include "SomaticRnaReport.h"
#include "SomaticReportHelper.h"
#include "TSVFileStream.h"


SomaticRnaReportData::SomaticRnaReportData(const SomaticReportSettings& other)
	: SomaticReportSettings(other)
{
}



SomaticRnaReport::SomaticRnaReport(const VariantList& snv_list, const CnvList& cnv_list, const SomaticRnaReportData& data)
	: db_()
	, data_(data)
{
	//filter DNA variants according somatic report configuration
	dna_snvs_ = SomaticRnaReportData::filterVariants(snv_list, data);
	dna_cnvs_ = SomaticRnaReportData::filterCnvs(cnv_list, data);


	//Get RNA processed sample name and resolve path to RNA directory
	ref_tissue_type_ = refTissueType(dna_snvs_);


	if(!QFile::exists(data.rna_fusion_file))
	{
		THROW(FileAccessException, "RNA fusions file does not exist: " + data.rna_fusion_file);
	}

	if(!checkRequiredSNVAnnotations(snv_list) && dna_snvs_.count() > 0)
	{
		THROW(FileParseException, "Missing annotation columns in variant list.");
	}
	if(!checkRequiredCNVAnnotations(cnv_list) && dna_cnvs_.count() > 0)
	{
		THROW(FileParseException, "Missing annotation columns in CNV list.");
	}


	//Load fusions
	try
	{
		TSVFileStream in_file(data.rna_fusion_file);
		int i_gene_left = in_file.colIndex("gene1", true);
		int i_gene_right = in_file.colIndex("gene2", true);
		int i_transcript_left = in_file.colIndex("transcript_id1", true);
		int i_transcript_right = in_file.colIndex("transcript_id2", true);
		int i_breakpoint_left = in_file.colIndex("breakpoint1", true);
		int i_breakpoint_right = in_file.colIndex("breakpoint2", true);
		int i_type = in_file.colIndex("type", true);
		while(!in_file.atEnd())
		{
			QByteArrayList parts = in_file.readLine();
			arriba_sv temp{parts[i_gene_left], parts[i_gene_right], parts[i_transcript_left], parts[i_transcript_right], parts[i_breakpoint_left], parts[i_breakpoint_right], parts[i_type]};
			svs_.append(temp);
		}
	}
	catch(Exception&)
	{
	}


}

bool SomaticRnaReport::checkRequiredSNVAnnotations(const VariantList& variants)
{
	//neccessary DNA annotations (exact match)
	const QByteArrayList an_names_dna = {"coding_and_splicing", "tumor_af"};
	for(QByteArray an : an_names_dna)
	{
		if(variants.annotationIndexByName(an, true, false) == -1) return false;
	}
	//neccessary RNA annotation (from RNA samples), (no exact match, because multiple RNA annotations possible)
	const QByteArrayList an_names_rna = {"_rna_depth", "_rna_af", "_rna_tpm", "_hpa_tissue_tpm", "_cohort_mean_tpm", "_log2fc", "_pvalue"};
	for(QByteArray an : an_names_rna)
	{
		if(variants.annotationIndexByName(an,false, false) == -1) return false;
	}

	return true;
}

bool SomaticRnaReport::checkRequiredCNVAnnotations(const CnvList &cnvs)
{
	QByteArrayList an_names_dna = {"cnv_type"};
	for(QByteArray an : an_names_dna)
	{
		if(cnvs.annotationIndexByName(an, false) < 0) return false;
	}

	QByteArrayList an_names_rna = {"_rna_data"};
	for(QByteArray an : an_names_rna)
	{
		if(cnvs.annotationIndexByName(an, false, true) == -1) return false;
	}

	return true;
}

QString SomaticRnaReport::refTissueType(const VariantList &variants)
{
	QString ref_tissue_type = "";
	for(QString comment: variants.comments())
	{
		if(comment.contains("RNA_REF_TPM_TISSUE="))
		{
			ref_tissue_type = comment.split('=')[1];
			break;
		}
	}
	return ref_tissue_type;
}

void SomaticRnaReport::checkRefTissueTypeInNGSD(QString ref_type, QString tumor_dna_ps_id)
{
	QString sample_id = db_.getValue("SELECT ps.sample_id FROM processed_sample as ps  WHERE ps.id = " + db_.processedSampleId(tumor_dna_ps_id) ).toString();
	QList<SampleDiseaseInfo> disease_info = db_.getSampleDiseaseInfo(sample_id, "RNA reference tissue");

	if(disease_info.count() != 1)
	{
		THROW(DatabaseException, "Found multiple or no RNA reference tissue for processed sample id " + tumor_dna_ps_id + " in NGSD.");
	}
	if(disease_info[0].disease_info != ref_type)
	{
		THROW(DatabaseException, "Found RNA reference tissue " + disease_info[0].disease_info + " in NGSD but " + ref_type + " in somatic GSVAR file.");
	}
}

int SomaticRnaReport::rankCnv(double tpm, double mean_tpm, SomaticGeneRole::Role gene_role)
{
	if(!BasicStatistics::isValidFloat(tpm) || !BasicStatistics::isValidFloat(mean_tpm)) return 2;

	double ratio = tpm/mean_tpm;

	if(gene_role == SomaticGeneRole::Role::LOSS_OF_FUNCTION && ratio <= 0.8)
	{
		return 1;
	}
	else if(gene_role == SomaticGeneRole::Role::ACTIVATING && ratio >= 1.5)
	{
		return 1;
	}
	return 2;
}


RtfTable SomaticRnaReport::partFusions()
{
	RtfTable fusion_table;
	fusion_table.addRow(RtfTableRow("Fusionen", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1));

	fusion_table.addRow(RtfTableRow({"Strukturvariante", "Transkript links", "Bruchpunkt Gen 1", "Transkript rechts", "Bruchpunkt Gen 2", "Typ"},{1600,1800,1400,1800,1400,1921}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());

	for(const auto& sv : svs_)
	{
		if( !sv.type.contains("translocation") && !sv.type.contains("inversion") ) continue;

		RtfTableRow temp;

		temp.addCell( 1600, sv.gene_left + "::" + sv.gene_right, RtfParagraph().setItalic(true).setFontSize(16) );
		temp.addCell( 1800, sv.transcipt_left, RtfParagraph().setFontSize(16) );
		temp.addCell( 1400, sv.breakpoint_left , RtfParagraph().setFontSize(16) );
		temp.addCell( 1800, sv.transcipt_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 1400, sv.breakpoint_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 1921, sv.type, RtfParagraph().setFontSize(16) );
		fusion_table.addRow( temp );
	}

	fusion_table.setUniqueBorder(1,"brdrhair",2);

	return fusion_table;
}


RtfSourceCode SomaticRnaReport::partFusionPics()
{
	QByteArrayList out;

	out << RtfParagraph("Strukturvarianten").setFontSize(18).setBold(true).RtfCode();

	RtfSourceCode desc = "Abbildung 1: Gezeigt werden die mögliche Strukturvarianten als lineare und als kreisförmige Genom-Darstellung, ";
	desc += "beteiligte Chromosomen, Fusionspartner, ihre Orientierung, Bruchpunkte im Genom, beteiligte Exons und ihre Abdeckung aus den Sequenzierdaten, das Fusionstranskript, ";
	desc += "Anzahl der für die Detektion unterstützenden Reads und Funktionelle Domäne im Protein mit der Position der Exons. ";
	desc += "Für Strukturvarianten innerhalb eines Gens wird zu Visualisierungszwecken eine Kopie des Gens zusätzlich dargestellt. Die Grafik wurde mit der Software ARRIBA erstellt.";

	out << RtfParagraph(desc).setHorizontalAlignment("j").setFontSize(16).RtfCode();

	for(const auto& pic_data : data_.fusion_pics)
	{
		out << pngToRtf(pic_data, doc_.maxWidth() - 500).RtfCode() << RtfParagraph("").RtfCode();
	}
	return out.join("\n");
}

RtfSourceCode SomaticRnaReport::partExpressionPics()
{
	QByteArrayList out;

	out << RtfParagraph("Expression bestimmter Gene aus therapierelevanten Signalkaskaden").setFontSize(18).setBold(true).RtfCode();

	RtfSourceCode desc = "Abbildung 2: Die Abbildung zeigt die jeweilige Genexpression als logarithmierten TPM in der Patientenprobe (";
	desc += RtfText("X").setFontSize(16).setFontColor(5).RtfCode();
	desc += "), in der Vergleichskohorte gleicher Tumorentität (Boxplot mit Quartil, SD und individuelle Expressionswerte) und als Mittelwert von Normalgewebe in der Literatur (" + RtfText("□").setFontSize(16).setBold(true).RtfCode() + ", Human Protein Altas). ";
	desc += "Die angegebenen Expressionswerte hängen unter anderem vom Tumorgehalt ab und sind daher nur mit Vorbehalt mit anderen Proben vergleichbar. ";
	desc += "Dargestellt sind die in Hinblick auf eine Therapie wichtigsten Signalkaskaden. Weitere Daten können auf Anfrage zur Verfügung gestellt werden. )";
	desc += "\n\\line\n";

	out << RtfParagraph(desc).setFontSize(16).setHorizontalAlignment("j").RtfCode();

	for(int i=0; i<data_.expression_plots.count(); ++i)
	{
		out << pngToRtf(data_.expression_plots[i], doc_.maxWidth() / 2 - 50).RtfCode();
		if((i+1)%2==0) out << RtfParagraph("").RtfCode();
	}

	return out.join("\n");
}

RtfTable SomaticRnaReport::partSVs()
{
	RtfTable fusion_table;
	fusion_table.addRow(RtfTableRow("Strukturvariante", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1));

	fusion_table.addRow(RtfTableRow({"Gen", "Transkript", "Bruchpunkt 1", "Bruchpunkt 2", "Beschreibung"},{1600,1800,1400,1800,3321}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());

	for(const auto& sv : svs_)
	{
		if ( !sv.type.contains("duplication") && !sv.type.contains("deletion")) continue;

		RtfTableRow temp;

		temp.addCell( 1600, sv.gene_right, RtfParagraph().setItalic(true).setFontSize(16) );
		temp.addCell( 1800, sv.transcipt_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 1400, sv.breakpoint_left , RtfParagraph().setFontSize(16) );
		temp.addCell( 1800, sv.breakpoint_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 3321, sv.type, RtfParagraph().setFontSize(16));

		fusion_table.addRow( temp );
	}
	fusion_table.setUniqueBorder(1,"brdrhair",2);

	return fusion_table;
}


RtfTable SomaticRnaReport::partSnvTable()
{
	RtfTable table;

	int i_co_sp = dna_snvs_.annotationIndexByName("coding_and_splicing");
	int i_tum_af = dna_snvs_.annotationIndexByName("tumor_af");

	int i_rna_tpm = dna_snvs_.annotationIndexByName(data_.rna_ps_name + "_rna_tpm");
	int i_rna_af = dna_snvs_.annotationIndexByName(data_.rna_ps_name + "_rna_af");

	int i_hpa_tissue_tpm = dna_snvs_.annotationIndexByName(data_.rna_ps_name + "_hpa_tissue_tpm");
	int i_cohort_mean_tpm = dna_snvs_.annotationIndexByName(data_.rna_ps_name + "_cohort_mean_tpm");
	int i_pval = dna_snvs_.annotationIndexByName(data_.rna_ps_name + "_pvalue");


	for(int i=0; i<dna_snvs_.count(); ++i)
	{
		const Variant& var = dna_snvs_[i];


		if(db_.getSomaticViccId(var) == -1) continue;
		SomaticViccData vicc_data = db_.getSomaticViccData(var);
		SomaticVariantInterpreter::Result vicc_result = SomaticVariantInterpreter::viccScore(vicc_data);
		if(vicc_result != SomaticVariantInterpreter::Result::ONCOGENIC && vicc_result != SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC) continue;

		VariantTranscript trans = SomaticReportHelper::selectSomaticTranscript(var, data_, i_co_sp);

		RtfTableRow row;

		//DNA data
		row.addCell(800, trans.gene, RtfParagraph().setItalic(true).setBold(true).setFontSize(16));//4400
		row.addCell({RtfText(trans.hgvs_c + ":" + trans.hgvs_p).setFontSize(16).RtfCode(), RtfText(trans.id).setFontSize(14).RtfCode()}, 1800);
		row.addCell(1200, trans.type.replace("_variant",""), RtfParagraph().setFontSize(16));
		row.addCell(600, QByteArray::number(var.annotations()[i_tum_af].toDouble(), 'f', 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

		//RNA data
		row.addCell( 600, var.annotations()[i_rna_af], RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		row.addCell( 1100, formatDigits(var.annotations()[i_rna_tpm]) , RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		row.addCell( 1100, formatDigits(var.annotations()[i_hpa_tissue_tpm]), RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		row.addCell( 900, formatDigits(var.annotations()[i_cohort_mean_tpm]), RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );

		double log_change = var.annotations()[i_rna_tpm].toDouble() / var.annotations()[i_cohort_mean_tpm].toDouble();
		row.addCell( 1000, (BasicStatistics::isValidFloat(log_change) ? QByteArray::number(log_change, 'f', 1) : "n/a" ), RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );

		row.addCell( 821, var.annotations()[i_pval], RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );

		for(int i=4; i<row.count(); ++i) row[i].setBackgroundColor(4);
		table.addRow(row);
	}
	table.sortByCol(0);

	RtfTableRow header = RtfTableRow({"Gen", "Veränderung", "Typ", "Anteil", "Anteil", "Tumorprobe TPM", "Referenz HPA-TPM", "Tumortyp MW-TPM", "Veränderung (x-fach)", "p-Wert"},{800,1800,1200,600,600,1100,1100, 900, 1000, 821}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader().setBorders(1, "brdrhair", 2);
	for(int i=4; i<header.count(); ++i) header[i].setBackgroundColor(4);
	table.prependRow( header );
	RtfTableRow sub_header = RtfTableRow({"DNA", "RNA"}, {4400, 5521}, RtfParagraph().setFontSize(16).setHorizontalAlignment("c").setBold(true)).setBorders(1, "brdrhair", 2);
	sub_header[1].setBackgroundColor(4);
	table.prependRow(sub_header);
	table.prependRow(RtfTableRow("Punktmutationen (SNVs) und kleine Insertionen/Deletionen (INDELs) (" + data_.rna_ps_name.toUtf8() + "-" + data_.tumor_ps.toUtf8() + "-" + data_.normal_ps.toUtf8() + ")", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.setUniqueBorder(1, "brdrhair", 2);

	return table;
}

RtfTable SomaticRnaReport::partCnvTable()
{
	int i_tum_clonality = dna_cnvs_.annotationIndexByName("tumor_clonality", true);
	int i_size_desc = dna_cnvs_.annotationIndexByName("cnv_type", true);
	//RNA annotations indices
	int i_rna_data = dna_cnvs_.annotationIndexByName(data_.rna_ps_name.toUtf8() + "_rna_data", true);



	RtfTable table;
	for(int i=0; i<dna_cnvs_.count(); ++i)
	{
		const CopyNumberVariant& cnv = dna_cnvs_[i];

		GeneSet genes = dna_cnvs_[i].genes().intersect(data_.target_region_filter.genes);

		for(const auto& gene : genes)
		{
			if(db_.getSomaticGeneRoleId(gene) == -1 ) continue;
			SomaticGeneRole role = db_.getSomaticGeneRole(gene, true);

			if( !SomaticCnvInterpreter::includeInReport(dna_cnvs_,cnv, role) ) continue;
			if( !role.high_evidence) continue;


			RtfTableRow temp;
			temp.addCell(800, gene, RtfParagraph().setBold(true).setItalic(true).setFontSize(16));
			temp.addCell(1800, cnv.chr().str() + " (" + cnv.annotations().at(i_size_desc).trimmed() + ")", RtfParagraph().setFontSize(16));

			int tumor_cn = cnv.copyNumber(dna_cnvs_.annotationHeaders());
			temp.addCell(1200, SomaticReportHelper::CnvTypeDescription(tumor_cn), RtfParagraph().setFontSize(16));

			temp.addCell(600, QByteArray::number(cnv.annotations().at(i_tum_clonality).toDouble(), 'f', 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

			const QByteArray& rna_data_field = cnv.annotations().at(i_rna_data);
			double tpm = getRnaData(gene, rna_data_field , "tpm");
			temp.addCell(1000, (BasicStatistics::isValidFloat(tpm) ? QByteArray::number( tpm, 'f', 2 ) : "n/a"), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

			double hpa_tissue_tpm = getRnaData(gene, rna_data_field, "hpa_tissue_tpm");
			temp.addCell(900, (BasicStatistics::isValidFloat(hpa_tissue_tpm) ? QByteArray::number( hpa_tissue_tpm, 'f', 2 ) : "n/a"), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

			//Determine oncogenetic rank of expression change
			int rank = rankCnv( tpm , hpa_tissue_tpm, role.role );
			temp.addCell( 900 , QByteArray::number(rank) , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

			double cohort_mean_tpm = getRnaData(gene, rna_data_field, "cohort_mean");
			temp.addCell(900, (BasicStatistics::isValidFloat(cohort_mean_tpm) ? QByteArray::number( cohort_mean_tpm, 'f', 2 ) : "n/a"), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

			//double log2fc = getRnaData(gene, rna_data_field, "log2fc");
			double fold_change = tpm / cohort_mean_tpm;
			temp.addCell(1000, (BasicStatistics::isValidFloat(fold_change) ? QByteArray::number( fold_change, 'f', 1 ) : "n/a") , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

			double pval = getRnaData(gene, rna_data_field, "pval");
			temp.addCell(821, (BasicStatistics::isValidFloat(pval) ? QByteArray::number( pval, 'f', 2 ) : "n/a") , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

			for(int i=4; i< temp.count(); ++i) temp[i].setBackgroundColor(4);

			table.addRow(temp);
		}
	}

	table.sortbyCols({8,0}); //sort by rank tean by gene symbol

	RtfTableRow header = RtfTableRow({"Gen", "Position", "CNV", "Anteil", "Tumorprobe TPM", "Referenz HPA-TPM", "Bewertung", "Tumortyp MW TPM", "Veränderung (x-fach)", "p-Wert"},{800, 1800, 1200, 600, 1000, 900,900, 900, 1000, 821}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader();
	for(int i=4; i< header.count(); ++i) header[i].setBackgroundColor(4);
	table.prependRow(header);

	RtfTableRow subheader = RtfTableRow({"DNA", "RNA"},{4400, 5521}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader();
	subheader[1].setBackgroundColor(4);
	table.prependRow( subheader);
	table.prependRow(RtfTableRow("Kopienzahlveränderungen (CNVs)", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.setUniqueBorder(1, "brdrhair", 2);

	return table;
}

RtfParagraph SomaticRnaReport::partVarExplanation()
{

	auto bold = [](RtfSourceCode text)
	{
		return RtfText(text).setBold(true).setFontSize(16).RtfCode();
	};

	RtfSourceCode out = "";

	out += bold("Veränderung:");

	out += " Kodierende Position, ";
	out += bold("SNV:");
	out += " Punktmutationen (Single Nucleotide Variant), ";
	out += bold("InDel:");
	out += " Insertionen/Deletionen, ";
	out += bold("CNV:") + " Kopienzahlvariante (Copy Number Variant), ";
	out += bold("AMP:") + " Amplifikation, " + bold("DEL:") + " Deletion, " + bold("LOH:") + " Kopienzahlneutraler Verlust der Heterozygotie (Loss of Heterozygosity), ";
	out += "WT: Wildtypallel, MUT: mutiertes Allel; ";
	out += bold("Typ:") + " Art der SNV oder Größe und Ausdehnung der CNV: focal (bis zu drei Gene), Cluster (weniger als 25% des Chromosomenarms), non-focal (großer Anteil des Chromosoms); ";
	out += bold("Anteil:") + " Anteil der Allele mit der gelisteten Variante (SNV, INDEL) bzw. Anteil derZellen mit der entsprechenden CNV in der untersuchten Probe; ";
	out += bold("CN:") + " Copy Number, ";
	out += bold("TPM:") + " Normalisierte Expression des Gens als Transkriptanzahl pro Kilobase und pro Million Reads. ";
	out += bold("Referenz HSA-TPM: ") + "Expression des Gens als Mittelwert TPM in Vergleichsproben aus Zellen aus der Großhirnrinde (The Human Protein Atlas).";
	out += bold("n/a:") + " Falls keine geeignete Referenzprobe vorhanden. ";
	out += bold("Bewertung (1):") + " Die Expression eines Gens mit beschriebenem Funtionsgewinn (Gain of Function) ist in der Probe erhöht oder die Expression eines Gens mit Funktionsverlust (LoF) ist in der Probe reduziert. ";
	out += bold("(2):") + " Die Expression ist in der Probe und in der Kontrolle ähnlich oder die Rolle des Gens in der Onkogenese ist nicht eindeutig. ";
	out += bold("Tumortyp MW-TPM:") + " Expression des Gens als Mittelwert TPM in Tumorproben gleicher Entität (in-house Datenbank). Bei weniger als fünf Tumorproben gleicher Entität ist kein Vergleich sinnvoll (n/a). ";
	out += bold("Veränderung (-fach):") + " Relative Expression in der Tumorprobe gegenüber der mittleren Expression in der in-house Vergleichskohorte gleicher Tumorentiät. ";
	out += bold("p-Wert:") + " Signifikanztest nach Fisher. " + bold("n/a:") + " nicht anwendbar.";

	return RtfParagraph(out).setFontSize(16).setHorizontalAlignment("j");
}

RtfPicture SomaticRnaReport::pngToRtf(std::tuple<QByteArray, int, int> tuple, int width_goal)
{
	QByteArray data;
	int width, height;
	std::tie(data,width,height) = tuple;

	//magnification ratio if pic resized to max width of document
	double ratio = (double)width_goal/ width;
	return RtfPicture(data, width, height).setWidth(width_goal).setHeight(height * ratio);
}

double SomaticRnaReport::getRnaData(QByteArray gene, QString field, QString key)
{
	QStringList entries = field.split(',');

	//Extract data from rna_data column. Data is organized as GENE_SYMBOL (KEY1=VALUE1 KEY2=VALUE2 ...)
	for(QString entry : entries)
	{
		QList<QString> parts = entry.append(' ').split(' ');
		if(parts[0].toUtf8() == gene)
		{
			int start = entry.indexOf('(');
			int end = entry.indexOf(')');
			QStringList data_entries = entry.mid(start+1, end-start-1).split(' '); //data from gene between brackets (...)
			for(QString data_entry : data_entries)
			{
				QStringList res = data_entry.append('=').split('=');
				if(res[0] == key)
				{
					bool ok = false;
					double res_value = res[1].toDouble(&ok);
					if(ok) return res_value;
					else return std::numeric_limits<double>::quiet_NaN();
				}
			}
		}
	}

	return std::numeric_limits<double>::quiet_NaN();
}

RtfSourceCode SomaticRnaReport::trans(QString orig_entry) const
{
	static QHash<QString, RtfSourceCode> dict;

	if(dict.isEmpty())
	{
		dict["adipose tissue"] = "Fettgewebe";
		dict["adrenal gland"] = "Nebenniere";
		dict["appendix"] = "Blinddarm";
		dict["B-cells"] = "B-Zellen";
		dict["bone marrow"] = "Knochenmark";
		dict["breast"] = "Brust";
		dict["cerebral cortex"] = "Großhirnrinde";
		dict["cervix, uterine"] = "Gebärmutterhals";
		dict["colon"] = "Dickdarm";
		dict["dendritic cells"] = "dendritische Zellen";
		dict["duodenum"] = "Zwölffingerdarm";
		dict["endometrium"] = "Endometrium";
		dict["epididymis"] = "Nebenhoden";
		dict["esophagus"] = "Speiseröhre";
		dict["fallopian tube"] = "Eileiter";
		dict["gallbladder"] = "Gallenblase";
		dict["granulocytes"] = "Granulozyten";
		dict["heart muscle"] = "Herzmuskel";
		dict["kidney"] = "Niere";
		dict["liver"] = "Leber";
		dict["lung"] = "Lunge";
		dict["lymph node"] = "Lymphknoten";
		dict["monocytes"] = "Monozyten";
		dict["NK-cells"] = "NK-Zellen";
		dict["ovary"] = "Eierstock";
		dict["pancreas"] = "Pankreas";
		dict["parathyroid gland"] = "Nebenschilddrüse";
		dict["placenta"] = "Plazenta";
		dict["prostate"] = "Prostata";
		dict["rectum"] = "Rektum";
		dict["salivary gland"] = "Speicheldrüse";
		dict["seminal vesicle"] = "Bläschendrüse";
		dict["skeletal muscle"] = "Skelettmuskel";
		dict["skin"] = "Haut";
		dict["small intestine"] = "Dünndarm";
		dict["smooth muscle"] = "glatter Muskel";
		dict["spleen"] = "Milz";
		dict["stomach"] = "Magen";
		dict["T-cells"] = "T-Zellen";
		dict["testis"] = "Hoden";
		dict["thyroid gland"] = "Schilddrüse";
		dict["tonsil"] = "Tonsilien";
		dict["urinary bladder"] = "Harnblase";
		dict["activating"] = "aktivierend";
		dict["likely_activating"] = "möglicherweise aktivierend";
		dict["inactivating"] = "inaktivierend";
		dict["likely_inactivating"] = "möglicherweise inaktivierend";
		dict["unclear"] = "unklare Bedeutung";
		dict["test_dependent"] = "testabhängige Bedeutung";
	}

	if(!dict.contains(orig_entry)) //Return highlighted original entry if not found
	{
		return RtfText(orig_entry.toUtf8()).highlight(3).setItalic(true).RtfCode();
	}

	return dict[orig_entry];
}

void SomaticRnaReport::writeRtf(QByteArray out_file)
{
	doc_.setMargins( RtfDocument::cm2twip(2.3) , 1134 , RtfDocument::cm2twip(1.2) , 1134 );
	doc_.setDefaultFontSize(16);


	doc_.addColor(191,191,191);
	doc_.addColor(161,161,161);
	doc_.addColor(255,255,0);
	doc_.addColor(242, 242, 242);
	doc_.addColor(255,0,0);

	if(dna_snvs_.count() > 0) doc_.addPart(partSnvTable().RtfCode());
	else doc_.addPart(RtfParagraph("Es wurden keine SNVs detektiert.").RtfCode());


	doc_.addPart(RtfParagraph("").RtfCode());

	if(dna_cnvs_.count() > 0) doc_.addPart(partCnvTable().RtfCode());
	else doc_.addPart(RtfParagraph("Es wurden keine CNVs detektiert.").RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());

	doc_.addPart(partVarExplanation().RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());

	if(svs_.count() > 0)
	{
		doc_.addPart(partFusions().RtfCode());
		doc_.addPart(RtfParagraph("").RtfCode());
	}

	if(svs_.count() > 0)
	{
		doc_.addPart(partSVs().RtfCode());
		doc_.newPage();
	}

	if(data_.fusion_pics.count() > 0)
	{
		doc_.addPart(partFusionPics());
		doc_.newPage();
	}

	if(data_.expression_plots.count() > 0)
	{
		doc_.addPart(partExpressionPics());
	}


	doc_.save(out_file);
}

RtfSourceCode SomaticRnaReport::formatDigits(QByteArray in, int digits)
{
	bool ok = false;
	RtfSourceCode tpm_formatted = QByteArray::number(in.toDouble(&ok), 'f', digits);
	if(!ok) tpm_formatted = "n/a";
	return tpm_formatted;
}
