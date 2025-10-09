#include <QCoreApplication>
#include <QDebug>
#include <cmath>
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
	//filter CNVs
	dna_cnvs_ = SomaticRnaReportData::filterCnvs(cnv_list, data);


	if(!VersatileFile(data.rna_fusion_file).exists())
	{
		THROW(FileAccessException, "RNA fusions file does not exist: " + data.rna_fusion_file);
	}
	if(!VersatileFile(data.rna_expression_file).exists())
	{
		THROW(FileAccessException, "RNA expression file does not exist: " + data.rna_expression_file);
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
		int i_reading_frame = in_file.colIndex("reading_frame", true);
		while(!in_file.atEnd())
		{
			QByteArrayList parts = in_file.readLine();
			arriba_sv temp{parts[i_gene_left], parts[i_gene_right], parts[i_transcript_left], parts[i_transcript_right], parts[i_breakpoint_left], parts[i_breakpoint_right], parts[i_type], parts[i_reading_frame]};
			svs_.append(temp);
		}
	}
	catch(Exception&)
	{
	}

	//Load pathway data from NGSD
	QByteArrayList pathway_genes;

	GeneSet genes_of_interest = GeneSet::createFromStringList({
								"ADAM12", "AKT1", "AKT2", "ATM", "ATR", "BAP1", "BRAF", "BRCA1", "BRCA2", "CCND1", "CCND2", "CCND3",
								"CCNE1", "CD274", "CDK12", "CDK4", "CDK6", "CDKN1A", "CDKN1B", "CDKN2A", "CDKN2B", "CDKN2C",
								"CDKN3B", "CHEK2", "EGFR", "EPHA3", "ERBB2", "ERCC1", "FANCA", "FANCC", "FANCD2", "FBXW7",
								"FGF19", "FGF3", "FGF4", "FGFR1", "FGFR2", "FGFR3", "FRS2", "INPP4B", "JAK2", "JUN", "KDR",
								"KIT", "MAP2K4", "MEN1", "MET", "MITF", "MLH1", "MSH2", "MTOR", "MYC", "NF1", "NF2", "PAK1",
								"PDGFRA", "PDPK1", "PIK3CA", "PTEN", "RB1", "RICTOR", "RPS6", "SETD2", "SH2B3", "SMARCA4",
								"SMARCB1", "STK11", "SYK", "TACSTD2", "TNFAIP3", "TOP2A", "TSC1", "TSC2"
	});

	genes_of_interest = db_.genesToApproved(genes_of_interest);

	for( QString& pathway : db_.getValues("SELECT CONCAT(spg.symbol, '\t', sp.name) FROM somatic_pathway_gene as spg, somatic_pathway as sp WHERE sp.id = spg.pathway_id") )
	{
		QByteArrayList parts = pathway.toUtf8().split('\t');
		ExpressionData tmp_data;
		tmp_data.symbol = parts[0];
		tmp_data.pathway = parts[1];

		tmp_data.role = db_.getSomaticGeneRole(parts[0]);

		if(!genes_of_interest.contains(tmp_data.symbol)) continue;

		pathways_ << tmp_data;
		if( !pathway_genes.contains(parts[0]) ) pathway_genes << parts[0];
	}

	//genes that have SNV/Indel/CNVs
	QByteArrayList genes_with_var;
	int i_co_sp = dna_snvs_.annotationIndexByName("coding_and_splicing");
	for(int i=0;i<dna_snvs_.count(); ++i)
	{
		QByteArray gene = SomaticReportHelper::selectSomaticTranscript(db_, dna_snvs_[i], i_co_sp).gene;
		if(!genes_with_var.contains(gene))
		{
			genes_with_var << gene;
		}
	}

	int i_cnv_type = dna_cnvs_.annotationIndexByName("cnv_type", true);
	for(int i=0; i<dna_cnvs_.count(); ++i)
	{
		GeneSet genes = dna_cnvs_[i].genes().intersect(data_.target_region_filter.genes);
		int cn = dna_cnvs_[i].copyNumber(dna_cnvs_.annotationHeaders());
		QByteArray cnv_type = dna_cnvs_[i].annotations()[i_cnv_type];

        for (const auto& gene : genes)
		{
			SomaticGeneRole role = db_.getSomaticGeneRole(gene);
			if (!role.isValid()) continue;

			if( !SomaticCnvInterpreter::includeInReport(cn, cnv_type, role) ) continue;
			if( !role.high_evidence) continue;

			if( !genes_with_var.contains(gene) ) genes_with_var << gene;
		}
	}

	//Get expression data for pathways andRNA sample from expression file
	TSVFileStream in_file(data.rna_expression_file);

	int i_gene = in_file.colIndex( "gene_name", true );
	int i_tpm = in_file.colIndex( "tpm", true );

	int i_hpa = -1;
	if (in_file.header().contains("hpa_tissue_tpm")) i_hpa = in_file.colIndex( "hpa_tissue_tpm", true);

	int i_cohort_mean = in_file.colIndex( "cohort_mean", true);
	int i_log2fc = in_file.colIndex("log2fc", true);
	int i_pvalue = in_file.colIndex("pval", true);

	auto toDouble = [](QByteArray in)
	{
		bool ok = false;
		double res = in.toDouble(&ok);
		if(ok) return res;
		else return std::numeric_limits<double>::quiet_NaN();
	};

	while( !in_file.atEnd() )
	{
		QByteArrayList parts = in_file.readLine();

		if( pathway_genes.contains(parts[i_gene]) )
		{
			for(ExpressionData& data : pathways_)
			{
				if(data.symbol != parts[i_gene]) continue;
				data.tumor_tpm = toDouble(parts[i_tpm]);
				if (i_hpa != -1) data.hpa_ref_tpm = toDouble(parts[i_hpa]);
				data.cohort_mean_tpm = toDouble(parts[i_cohort_mean]);
				data.log2fc = toDouble(parts[i_log2fc]);
				data.pvalue = toDouble(parts[i_pvalue]);
			}
		}

		if( genes_with_var.contains(parts[i_gene]) )
		{
			ExpressionData data;
			data.symbol = parts[i_gene];
			data.tumor_tpm = toDouble(parts[i_tpm]);
			if (i_hpa != -1) data.hpa_ref_tpm = toDouble(parts[i_hpa]);
			data.cohort_mean_tpm = toDouble(parts[i_cohort_mean]);
			data.log2fc = toDouble(parts[i_log2fc]);
			data.pvalue = toDouble(parts[i_pvalue]);

			expression_per_gene_.insert(parts[i_gene], data);
		}

		//Add highly confident expression
		if( toDouble(parts[i_pvalue]) < 0.05)
		{
			ExpressionData data;
			data.symbol = parts[i_gene];
			data.role = db_.getSomaticGeneRole(parts[i_gene]);
			data.tumor_tpm = toDouble(parts[i_tpm]);
			if (i_hpa != -1) data.hpa_ref_tpm = toDouble(parts[i_hpa]);
			data.cohort_mean_tpm = toDouble(parts[i_cohort_mean]);
			data.log2fc = toDouble(parts[i_log2fc]);
			data.pvalue = toDouble(parts[i_pvalue]);

			//Only use approved gene symbols
			if(db_.approvedGeneNames().contains(data.symbol)) high_confidence_expression_ << data;
			else
			{
				QByteArray approved_symbol = db_.geneToApproved(data.symbol);
				if(approved_symbol != "")
				{
					data.symbol = approved_symbol;
					high_confidence_expression_ << data;
				}
			}
		}
	}

	//sort high confident by logfold change
	std::sort(high_confidence_expression_.begin(), high_confidence_expression_.end(), [](const ExpressionData& rhs, const ExpressionData& lhs){return rhs.log2fc < lhs.log2fc;});
}

bool SomaticRnaReport::checkRequiredSNVAnnotations(const VariantList& variants)
{
	//neccessary DNA annotations (exact match)
	const QByteArrayList an_names_dna = {"coding_and_splicing", "tumor_af"};
    for (const QByteArray& an : an_names_dna)
	{
		if(variants.annotationIndexByName(an, true, false) == -1) return false;
	}
	return true;
}

bool SomaticRnaReport::checkRequiredCNVAnnotations(const CnvList &cnvs)
{
	QByteArrayList an_names_dna = {"cnv_type"};
    for (const QByteArray& an : an_names_dna)
	{
		if(cnvs.annotationIndexByName(an, false) < 0) return false;
	}

	return true;
}

int SomaticRnaReport::rank(double tpm, double mean_tpm, SomaticGeneRole::Role gene_role)
{
	if(!BasicStatistics::isValidFloat(tpm) || !BasicStatistics::isValidFloat(mean_tpm) || tpm < 10 ) return 3;

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

	fusion_table.addRow(RtfTableRow({"Strukturvariante", "Transkript links", "Bruchpunkt Gen 1", "Transkript rechts", "Bruchpunkt Gen 2", "Typ", "Leseraster"},{1600,1400,1400,1400,1400,1700, 1021}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());

    for (const auto& sv : svs_)
	{
		RtfTableRow temp;

		temp.addCell( 1600, sv.gene_left + "::" + sv.gene_right, RtfParagraph().setItalic(true).setFontSize(16) );
		temp.addCell( 1400, sv.transcipt_left, RtfParagraph().setFontSize(16) );
		temp.addCell( 1400, sv.breakpoint_left , RtfParagraph().setFontSize(16) );
		temp.addCell( 1400, sv.transcipt_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 1400, sv.breakpoint_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 1700, trans(sv.type), RtfParagraph().setFontSize(16) );
		temp.addCell( 1021, sv.reading_frame, RtfParagraph().setFontSize(16) );
		fusion_table.addRow( temp );
	}



	fusion_table.setUniqueBorder(1,"brdrhair",2);

	return fusion_table;
}


RtfSourceCode SomaticRnaReport::partFusionPics()
{
	QByteArrayList out;

	for(RtfPicture pic : data_.fusion_pics)
	{
		pic.resizeToWidth(doc_.maxWidth() - 500);

		out << pic.RtfCode();
		out << RtfParagraph("").RtfCode();
	}
	return out.join("\n");
}

RtfSourceCode SomaticRnaReport::partExpressionPics()
{
	QByteArrayList out;

	out << RtfParagraph("Expression von Genen aus ausgewählten therapierelevanten Signalkaskaden").setFontSize(18).setBold(true).RtfCode();

	RtfSourceCode desc = "Die Abbildung zeigt die jeweilige Genexpression als logarithmierten TPM in der Patientenprobe (";
	desc += RtfText("\\'d7").setFontSize(16).setFontColor(5).RtfCode();
	desc += "), in der Vergleichskohorte gleicher Tumorentität (Boxplot mit Quartil, SD und individuelle Expressionswerte) und als Mittelwert von Normalgewebe in der Literatur (" + RtfText("□").setFontSize(16).setBold(true).RtfCode() + ", Human Protein Altas) falls vorhanden. ";
	desc += "Die angegebenen Expressionswerte hängen unter anderem vom Tumorgehalt ab und sind daher nur mit Vorbehalt mit anderen Proben vergleichbar. ";
	desc += "Dargestellt sind die in Hinblick auf eine Therapie wichtigsten Signalkaskaden. Weitere Daten können auf Anfrage zur Verfügung gestellt werden.";
	desc += "\n\\line\n";

	out << RtfParagraph(desc).setFontSize(16).setHorizontalAlignment("j").RtfCode();

	for(int i=0; i<data_.expression_plots.count(); ++i)
	{
		RtfPicture pic = data_.expression_plots[i];
		pic.resizeToWidth(doc_.maxWidth() / 2 - 400);

		out << pic.RtfCode();
		if((i+1)%2==0) out << RtfParagraph("").RtfCode();
	}
	return out.join("\n");
}

RtfTable SomaticRnaReport::partSVs()
{
	RtfTable fusion_table;
	fusion_table.addRow(RtfTableRow("Strukturvarianten", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1));

	fusion_table.addRow(RtfTableRow({"Gen", "Transkript", "Bruchpunkt 1", "Bruchpunkt 2", "Beschreibung"},{1600,1800,1400,1800,3321}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());

    for (const auto& sv : svs_)
	{
		if ( ! (sv.type.contains("duplication") && sv.gene_left == sv.gene_right) && !sv.type.contains("deletion")) continue;

		RtfTableRow temp;

		temp.addCell( 1600, sv.gene_right, RtfParagraph().setItalic(true).setFontSize(16) );
		temp.addCell( 1800, sv.transcipt_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 1400, sv.breakpoint_left , RtfParagraph().setFontSize(16) );
		temp.addCell( 1800, sv.breakpoint_right, RtfParagraph().setFontSize(16) );
		temp.addCell( 3321, trans(sv.type), RtfParagraph().setFontSize(16));

		fusion_table.addRow( temp );
	}
	if(fusion_table.count() == 2) return RtfTable();

	fusion_table.setUniqueBorder(1,"brdrhair",2);

	return fusion_table;
}


RtfTable SomaticRnaReport::partSnvTable()
{
	RtfTable table;

	int i_co_sp = dna_snvs_.annotationIndexByName("coding_and_splicing");
	int i_tum_af = dna_snvs_.annotationIndexByName("tumor_af");

	BamReader bam_file(data_.rna_bam_file, data_.ref_genome_fasta_file);

	for(int i=0; i<dna_snvs_.count(); ++i)
	{
		const Variant& var = dna_snvs_[i];

		if(db_.getSomaticViccId(var) == -1) continue;
		SomaticViccData vicc_data = db_.getSomaticViccData(var);
		SomaticVariantInterpreter::Result vicc_result = SomaticVariantInterpreter::viccScore(vicc_data);
		if(vicc_result != SomaticVariantInterpreter::Result::ONCOGENIC && vicc_result != SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC) continue;

		VariantTranscript trans = SomaticReportHelper::selectSomaticTranscript(db_, var, i_co_sp);

		ExpressionData data = expression_per_gene_.value(trans.gene, ExpressionData());

		VariantDetails var_details = bam_file.getVariantDetails(FastaFileIndex(data_.ref_genome_fasta_file), var, false);

		RtfTableRow row;

		//DNA data
		row.addCell(800, trans.gene, RtfParagraph().setItalic(true).setBold(true).setFontSize(16));//4400
		if (trans.hgvs_c.length() == 0 && trans.hgvs_p.length() == 0)
		{
			row.addCell(1900, QByteArrayList() << RtfText("???").setFontSize(16).highlight(3).RtfCode() << RtfText(trans.id).setFontSize(14).RtfCode());
		}
		else
		{
			row.addCell(1900, QByteArrayList() << RtfText(trans.hgvs_c + ", " + trans.hgvs_p).setFontSize(16).RtfCode() << RtfText(trans.id).setFontSize(14).RtfCode());
		}

		row.addCell(1300, trans.type.replace("_variant",""), RtfParagraph().setFontSize(16));
		row.addCell(700, formatDigits(var.annotations()[i_tum_af].toDouble(), 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

		//RNA data
		if (var_details.depth > 4)
		{
			row.addCell( 700, formatDigits(var_details.frequency, 2), RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		}
		else
		{
			row.addCell( 700, "n/a", RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		}

		row.addCell( 1200, formatDigits(data.tumor_tpm) , RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		row.addCell( 1200, BasicStatistics::isValidFloat(data.hpa_ref_tpm) ? formatDigits(data.hpa_ref_tpm) : "-", RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		row.addCell( 1000, formatDigits(data.cohort_mean_tpm), RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );

		row.addCell( 1121, expressionChange(data) , RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );

		for(int i=4; i<row.count(); ++i) row[i].setBackgroundColor(4);
		table.addRow(row);
	}
	table.sortByCol(0);

	RtfTableRow header = RtfTableRow({"Gen", "Veränderung", "Typ", "Anteil", "Anteil", "Tumorprobe TPM", "Normalprobe TPM", "Tumortyp\n\\line\nMW-TPM", "Veränderung\n\\line\n(x-fach)"},{800,1900,1300,700,700,1200,1200, 1000, 1121}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader().setBorders(1, "brdrhair", 2);
	for(int i=4; i<header.count(); ++i) header[i].setBackgroundColor(4);
	table.prependRow( header );
	RtfTableRow sub_header = RtfTableRow({"DNA", "RNA"}, {4700, 5221}, RtfParagraph().setFontSize(16).setHorizontalAlignment("c").setBold(true)).setBorders(1, "brdrhair", 2);
	sub_header[1].setBackgroundColor(4);
	table.prependRow(sub_header);
	table.prependRow(RtfTableRow("Punktmutationen (SNVs) und kleine Insertionen/Deletionen (INDELs) (" + data_.rna_ps_name.toUtf8() + "-" + data_.tumor_ps.toUtf8() + "-" + data_.normal_ps.toUtf8() + ")", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.setUniqueBorder(1, "brdrhair", 2);

	return table;
}


RtfTable SomaticRnaReport::partCnvTable()
{
	int i_tum_clonality = dna_cnvs_.annotationIndexByName("tumor_clonality", true);
	int i_cnv_type = dna_cnvs_.annotationIndexByName("cnv_type", true);

	RtfTable table;

	for(int i=0; i<dna_cnvs_.count(); ++i)
	{
		const CopyNumberVariant& cnv = dna_cnvs_[i];
		int cn = cnv.copyNumber(dna_cnvs_.annotationHeaders());
		QByteArray cnv_type = cnv.annotations()[i_cnv_type];

		GeneSet genes = dna_cnvs_[i].genes().intersect(data_.target_region_filter.genes);

        for (const auto& gene : genes)
		{
			SomaticGeneRole role = db_.getSomaticGeneRole(gene, true);
			if (!role.isValid()) continue;

			if( !SomaticCnvInterpreter::includeInReport(cn, cnv_type, role) ) continue;
			if( !role.high_evidence) continue;

			ExpressionData expr_data = expression_per_gene_.value(gene, ExpressionData());

			RtfTableRow temp;
			temp.addCell(800, gene, RtfParagraph().setBold(true).setItalic(true).setFontSize(16));
			temp.addCell(1900, cnv.chr().str() + " (" + cnv_type.trimmed() + ")", RtfParagraph().setFontSize(16));

			int tumor_cn = cnv.copyNumber(dna_cnvs_.annotationHeaders());
			temp.addCell(1300, SomaticReportHelper::CnvTypeDescription(tumor_cn, true), RtfParagraph().setFontSize(16));
			temp.addCell(700, QByteArray::number(cnv.annotations().at(i_tum_clonality).toDouble(), 'f', 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));
			temp.addCell(1100, formatDigits( expr_data.tumor_tpm), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
			temp.addCell(1000, BasicStatistics::isValidFloat(expr_data.hpa_ref_tpm) ? formatDigits(expr_data.hpa_ref_tpm) : "-", RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
			temp.addCell( 1000 , QByteArray::number( rank( expr_data.tumor_tpm , expr_data.hpa_ref_tpm, role.role ) ) , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );


			//Determine oncogenetic rank of expression change
			temp.addCell(1000, formatDigits( expr_data.cohort_mean_tpm), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
			if ((expr_data.tumor_tpm > 10 && expr_data.cohort_mean_tpm > 10))
			{
				temp.addCell(1121, expressionChange(expr_data) , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
			}
			else
			{
				temp.addCell(1121, "-" , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
			}


			for(int i=4; i< temp.count(); ++i) temp[i].setBackgroundColor(4);

			table.addRow(temp);
		}
	}

	table.sortbyCols({6,0}); //sort by rank, then by gene symbol

	RtfTableRow header = RtfTableRow({"Gen", "Position", "CNV", "Anteil", "Tumorprobe TPM", "Normalprobe TPM", "Bewertung", "Tumortyp\n\\line\nMW-TPM", "Veränderung\n\\line\n(x-fach)"},{800, 1900, 1300, 700, 1100, 1000,1000, 1000, 1121, }, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader();
	for(int i=4; i< header.count(); ++i) header[i].setBackgroundColor(4);
	table.prependRow(header);

	RtfTableRow subheader = RtfTableRow({"DNA", "RNA"},{4700, 5221}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader();
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
	out += bold("Anteil:") + " Anteil der Allele mit der gelisteten Variante (SNV, INDEL) bzw. Anteil der Zellen mit der entsprechenden CNV in der untersuchten Probe; ";
	out += bold("CN:") + " Copy Number, ";
	out += bold("TPM:") + " Normalisierte Expression des Gens als Transkriptanzahl pro Kilobase und pro Million Reads. ";
	if (data_.rna_hpa_ref_tissue == "")
	{
		out += bold("Normalprobe TPM: ") + "Daten für eine geeignete Referenzprobe waren nicht verfügbar.";
	}
	else
	{
		out += bold("Normalprobe TPM: ") + "Expression des Gens als Mittelwert TPM in Vergleichsproben aus Zellen aus " + bold(trans(data_.rna_hpa_ref_tissue, 16)) + " (The Human Protein Atlas). ";
	}

	out += bold("Bewertung (1):") + " Die Expression eines Gens mit beschriebenem Funktionsgewinn (Gain of Function) ist in der Probe erhöht oder die Expression eines Gens mit Funktionsverlust (LoF) ist in der Probe reduziert im Vergleich zu Normalprobe TPM. ";
	out += bold("(2):") + " Die Expression ist in der Probe und in der Kontrolle ähnlich oder die Rolle des Gens in der Onkogenese ist nicht eindeutig. ";
	out += bold("(3):") + " Eine differenzielle Expression kann nicht bewertet werden. ";
	out += bold("Tumortyp MW-TPM:") + " Expression des Gens als Mittelwert TPM in Tumorproben gleicher Entität (in-house Datenbank). Bei weniger als fünf Tumorproben gleicher Entität ist kein Vergleich sinnvoll (n/a). ";
	out += bold("Veränderung (x-fach):") + " Relative Expression in der Tumorprobe gegenüber der mittleren Expression in der in-house Vergleichskohorte gleicher Tumorentität. " + bold("-:") + " Die Anzahl der Proben in der Tumorkohorte erlaubt keine statistische Bewertung oder die Expression ist niedrig. ";
	out += bold("*: p<0.05:") + " Signifikanztest nach Fisher. " + bold("n/a:") + " nicht anwendbar. ";

	return RtfParagraph(out).setFontSize(16).setHorizontalAlignment("j");
}

RtfTable SomaticRnaReport::partGeneExpression()
{
	RtfTable table;

	table.addRow( RtfTableRow("Expression ausgewählter Gene", 9921, RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.addRow(RtfTableRow({"Gen", "Pathogenität", "Signalweg", "Tumorprobe TPM", "Normalprobe TPM", "Bewertung", "Tumortyp\n\\line\nMW-TPM", "Veränderung\n\\line\n(x-fach)"},	{1237, 1237, 1958, 1137, 1137, 937, 1137, 1141}, RtfParagraph().setHorizontalAlignment("c").setBold(true)).setHeader().setBorders(1, "brdrhair", 2));
	for(int i=2; i<table[1].count(); ++i) table[1][i].setBackgroundColor(4);

	//Sort genes by gene name instead of pathway:
	std::sort(pathways_.begin(), pathways_.end(), [](ExpressionData a, ExpressionData b) {
		return a.symbol < b.symbol;
	});

    for (const auto& data : pathways_)
	{
		RtfTableRow row;
		row.addCell(1237, data.symbol );

		QByteArray pathogenicity = "-";
		if(data.role.role == SomaticGeneRole::Role::ACTIVATING) pathogenicity = "GoF";
		else if(data.role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION) pathogenicity = "LoF";

		row.addCell(1237, pathogenicity  );
		row.addCell(1958, data.pathway );
		row.addCell(1137, formatDigits(data.tumor_tpm), RtfParagraph().setHorizontalAlignment("c") );
		row.addCell(1137, BasicStatistics::isValidFloat(data.hpa_ref_tpm) ? formatDigits(data.hpa_ref_tpm) : "-", RtfParagraph().setHorizontalAlignment("c"));
		row.addCell(937, QByteArray::number(rank(data.tumor_tpm, data.hpa_ref_tpm, data.role.role)) , RtfParagraph().setHorizontalAlignment("c"));

		row.addCell(1137, formatDigits(data.cohort_mean_tpm) , RtfParagraph().setHorizontalAlignment("c"));
		if (data.tumor_tpm > 10 && data.cohort_mean_tpm > 10)
		{
			row.addCell(1141, expressionChange(data) , RtfParagraph().setHorizontalAlignment("c"));
		}
		else
		{
			row.addCell(1141, "-" , RtfParagraph().setHorizontalAlignment("c"));
		}


		row.setBorders(1, "brdrhair", 2);

		for(int i=2; i<row.count(); ++i) row[i].setBackgroundColor(4);

		table.addRow(row);
	}

	table.setUniqueFontSize(16);
	return table;
}

RtfParagraph SomaticRnaReport::partGeneExprExplanation()
{
	auto bold = [](RtfSourceCode text)
	{
		return RtfText(text).setBold(true).setFontSize(16).RtfCode();
	};

	RtfSourceCode out = "";

	out += bold("Pathogenität:");
	out += " DEL (Verlust) oder AMP (Zugewinn) der Funktion oder reduzierte bzw. erhöhte Expression ist Pathomechanismus, ";
	out += bold("TPM:") + " Normierte Expression des Gens als Transkripte pro Kilobase pro Million Reads. ";
	if (data_.rna_hpa_ref_tissue == "")
	{
		out += bold("Normalprobe TPM: ") + "Daten für eine geeignete Referenzkontrolle waren nicht verfügbar.";
	}
	else
	{
		out += bold("Normalprobe TPM:") + " Expression des Gens als Mittelwert TPM in Vergleichsproben von Zellen aus " + bold(trans(data_.rna_hpa_ref_tissue, 16)) + " (The Human Protein Atlas). ";
	}
	out += bold("Bewertung (1):") + " Die Expression eines Gens mit beschriebenem Funktionsgewinn (Gain of Function) ist in der Probe erhöht oder die Expression eines Gens mit Funktionsverlust (LoF) ist in der Probe reduziert im Vergleich zur Normalprobe. ";
	out += bold("(2):") + " Die Expression ist in der Probe und in der Kontrolle ähnlich oder die Rolle des Gens in der Onkogenese ist nicht eindeutig. ";
	out += bold("(3):") + " Eine differenzielle Expression kann nicht bewertet werden. ";
	out += bold("Tumortyp MW-TPM:") + " Expression des Gens als Mittelwert TPM in Tumorproben gleicher Entität (in-house Datenbank). Bei weniger als fünf Tumorproben gleicher Entität ist kein Vergleich sinnvoll (-). ";
	out += bold("Veränderung (x-fach): ") + " Relative Expression in der Tumorprobe gegenüber der mittleren Expression in der in-house Vergleichskohorte gleicher Tumorentität. ";
	out += bold("*: p<0.05") + " (Signifikanztest nach Fisher). " + bold("n/a:") + " nicht anwendbar. " + bold("-:") + " Die Anzahl der Proben in der Tumorkohorte erlaubt keine statistische Bewertung";

	return RtfParagraph(out).setFontSize(16).setHorizontalAlignment("j");
}

RtfSourceCode SomaticRnaReport::partTop10Expression()
{
	RtfTable table;

	QList<ExpressionData> activating_genes;
	QList<ExpressionData> lof_genes;

    for (const auto& data : high_confidence_expression_)
	{
		if(data.role.role == SomaticGeneRole::Role::ACTIVATING		 && data.tumor_tpm >= 10 && data.cohort_mean_tpm > 10) activating_genes << data;
		if(data.role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION && data.tumor_tpm >= 10 && data.cohort_mean_tpm > 10) lof_genes << data;
	}

	//sort by log2fc, descending for activating expression, ascending for LoF genes
	std::sort(activating_genes.begin(), activating_genes.end(), [](const ExpressionData& rhs, const ExpressionData& lhs){return rhs.log2fc > lhs.log2fc;});
	std::stable_sort(activating_genes.begin(), activating_genes.end(), [](const ExpressionData& rhs, const ExpressionData& lhs){return rank( rhs.tumor_tpm , rhs.hpa_ref_tpm, rhs.role.role ) < rank( lhs.tumor_tpm , lhs.hpa_ref_tpm, lhs.role.role );});
	std::sort(lof_genes.begin(), lof_genes.end(), [](const ExpressionData& rhs, const ExpressionData& lhs){return rhs.log2fc < lhs.log2fc;});
	std::stable_sort(lof_genes.begin(), lof_genes.end(), [](const ExpressionData& rhs, const ExpressionData& lhs){return rank( rhs.tumor_tpm , rhs.hpa_ref_tpm, rhs.role.role ) < rank( lhs.tumor_tpm , lhs.hpa_ref_tpm, lhs.role.role );});

	QList<ExpressionData> genes_to_be_reported;
	genes_to_be_reported << activating_genes.mid(0, 10) << lof_genes.mid(0, 10);

	table.addRow( RtfTableRow("Top 10 Gene mit veränderter Expression", 9921, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );


	table.addRow(RtfTableRow({"Gen", "Pathogenität", "Tumorprobe TPM", "Normalprobe TPM", "Bewertung", "Tumortyp MW-TPM", "Veränderung (x-fach)"},	{1488, 1488, 1388, 1388, 1188, 1488, 1492}, RtfParagraph().setHorizontalAlignment("c").setFontSize(16).setBold(true)).setHeader().setBorders(1, "brdrhair", 2));
	for(int i=2; i<table.last().count(); ++i) table.last()[i].setBackgroundColor(4);


    for (const auto& data : genes_to_be_reported)
	{
		RtfTableRow row;

		row.addCell( 1488, data.symbol, RtfParagraph().setItalic(true).setFontSize(16).setHorizontalAlignment("c") );

		QByteArray mode = "n/a";
		if(data.role.role == SomaticGeneRole::Role::ACTIVATING) mode = "GoF";
		else if (data.role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION) mode = "LoF";
		row.addCell( 1488, mode, RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

		row.addCell( 1388, formatDigits(data.tumor_tpm), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

		row.addCell( 1388, BasicStatistics::isValidFloat(data.hpa_ref_tpm) ? formatDigits(data.hpa_ref_tpm) : "-", RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

		row.addCell( 1188 , QByteArray::number( rank( data.tumor_tpm , data.hpa_ref_tpm, data.role.role ) ), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

		row.addCell( 1488, formatDigits(data.cohort_mean_tpm), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );

		row.addCell( 1492, formatDigits(std::pow(2., data.log2fc), 1), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );


		for(int i=2; i<row.count(); ++i) row[i].setBackgroundColor(4);

		row.setBorders(1, "brdrhair", 2);

		table.addRow(row);
	}

	if(table.count() == 2) return RtfParagraph("").RtfCode();


	auto bold = [](RtfSourceCode text)
	{
		return RtfText(text).setBold(true).setFontSize(16).RtfCode();
	};

	RtfSourceCode expl = bold("TPM:") + " Normierte Expression des Gens als Transkripte pro Kilobase pro Million Reads. ";
	if (data_.rna_hpa_ref_tissue == "")
	{
		expl += bold("Normalprobe TPM: ") + "Daten für eine geeignete Referenzprobe waren nicht verfügbar.";
	} else
	{
		expl += bold("Normalprobe TPM:") + " Expression des Gens als Mittelwert TPM in Vergleichsproben von Zellen aus " + bold(trans(data_.rna_hpa_ref_tissue, 16)) + " (The Human Protein Atlas). ";
	}
	expl += bold("Bewertung (1):") + " Die Expression eines Gens mit beschriebenem Funktionsgewinn (Gain of Function) ist in der Probe erhöht oder die Expression eines Gens mit Funktionsverlust (LoF) ist in der Probe reduziert im Vergleich zu Normalprobe TPM. ";
	expl += bold("(2):") + " Die Expression ist in der Probe und in der Kontrolle ähnlich oder die Rolle des Gens in der Onkogenese ist nicht eindeutig. ";
	expl += bold("(3):") + " Eine differenzielle Expression kann nicht bewertet werden. ";
	expl += bold("Tumortyp MW-TPM:") + " Expression des Gens als Mittelwert TPM in Tumorproben gleicher Entität (in-house Datenbank). ";
	expl += bold("Veränderung (x-fach): ") + "Relative Expression in der Tumorprobe gegenüber der mittleren Expression in der in-house Vergleichskohorte gleicher Tumorentität.";


	RtfSourceCode intro = RtfParagraph("Top 10 Genlisten mit signifikant veränderter Expression").setFontSize(18).setBold(true).RtfCode();
	intro += RtfParagraph("Die Tabelle zeigt bis zu 10 Onkogene, deren relative Expression in der Tumorprobe gegenüber der mittleren Expression in der in-house Vergleichskohorte gleicher Tumorentität am höchsten ist. Die Liste enthält nur Gene, mit Tumortyp MW TPM > 10 und einem p-Wert < 0.05. Die Tabelle enthält weiterhin diejenigen 10 Tumorsuppressorgene mit Funktionsverlust (LoF), deren Expression gegenüber der Vergleichskohorte signifikant am niedrigsten ist und Tumor TPM > 10 ist.").setFontSize(16).setHorizontalAlignment("j").RtfCode();

	return intro + "\n" + table.RtfCode() + "\n" + RtfParagraph(expl).setFontSize(16).setHorizontalAlignment("j").RtfCode();
}

RtfTable SomaticRnaReport::partGeneralInfo()
{
	RtfTable table;

	table.addRow( RtfTableRow({"Allgemeine Informationen", "Qualitätsparameter"}, {5061, 4861}, RtfParagraph().setFontSize(18).setBold(true)).setHeader() );

	table.addRow( RtfTableRow( {"Auswertungsdatum:", data_.report_config->evaluationDate().toString("dd.MM.yyyy").toUtf8(), "Analysepipeline:", dna_snvs_.getPipeline().toUtf8()}, {2000,3061,2500,2361}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Proben-ID (Tumor-DNA):", data_.tumor_ps.toUtf8(), "Auswertungssoftware:",  QCoreApplication::applicationName().toUtf8() + " " + QCoreApplication::applicationVersion().toUtf8()}, {2000,3061,2500,2361}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Proben-ID (Tumor-RNA):", data_.rna_ps_name.toUtf8(), "Anzahl Reads ", data_.rna_qcml_data.value("QC:2000005",true).toString().toUtf8()}, {2000,3061,2500,2361}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Prozessierungssystem:", db_.getProcessingSystemData( db_.processingSystemIdFromProcessedSample(data_.rna_ps_name) ).name.toUtf8(), "On-Target Read Percentage:", data_.rna_qcml_data.value("QC:2000021",true).toString().toUtf8() + "\%"}, {2000,3061,2500,2361}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"ICD10:", data_.icd10.toUtf8(), "Target Region Read Depth:", data_.rna_qcml_data.value("QC:2000025",true).toString().toUtf8() +"x"}, {2000,3061,2500,2361}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Tumortyp:", data_.phenotype.toUtf8(), "House Keeping Genes 10x Percentage:", data_.rna_qcml_data.value("QC:2000102",true).toString().toUtf8() + "%"}, {2000,3061,2500,2361}, RtfParagraph().setFontSize(14)) );
	table.addRow( RtfTableRow( {"Korrelation der Expression mit der Tumorentität:", QByteArray::number(data_.expression_correlation, 'f', 2) + " (n=" + QByteArray::number(data_.cohort_size) + ")" , "", ""}, {2000,3061,2500,2361}, RtfParagraph().setFontSize(14) ) );

	return table;
}

double SomaticRnaReport::getRnaData(QByteArray gene, QString field, QString key)
{
	QStringList entries = field.split(',');

	//Extract data from rna_data column. Data is organized as GENE_SYMBOL (KEY1=VALUE1 KEY2=VALUE2 ...)
    for (QString entry : entries)
	{
		QList<QString> parts = entry.append(' ').split(' ');
		if(parts[0].toUtf8() == gene)
		{
			int start = entry.indexOf('(');
			int end = entry.indexOf(')');
			QStringList data_entries = entry.mid(start+1, end-start-1).split(' '); //data from gene between brackets (...)
            for (QString data_entry : data_entries)
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

RtfTable SomaticRnaReport::uncertainSnvTable()
{
	RtfTable table;

	int i_co_sp = dna_snvs_.annotationIndexByName("coding_and_splicing");
	int i_tum_af = dna_snvs_.annotationIndexByName("tumor_af");

	BamReader bam_file(data_.rna_bam_file, data_.ref_genome_fasta_file);

	for(int i=0; i<dna_snvs_.count(); ++i)
	{
		const Variant& var = dna_snvs_[i];

		if(db_.getSomaticViccId(var) == -1) continue;
		SomaticViccData vicc_data = db_.getSomaticViccData(var);
		SomaticVariantInterpreter::Result vicc_result = SomaticVariantInterpreter::viccScore(vicc_data);
		if(vicc_result != SomaticVariantInterpreter::Result::UNCERTAIN_SIGNIFICANCE) continue;

		VariantTranscript trans = SomaticReportHelper::selectSomaticTranscript(db_, var, i_co_sp);

		ExpressionData data = expression_per_gene_.value(trans.gene, ExpressionData());

		VariantDetails var_details = bam_file.getVariantDetails(FastaFileIndex(data_.ref_genome_fasta_file), var, false);

		RtfTableRow row;

		//DNA data
		row.addCell(800, trans.gene, RtfParagraph().setItalic(true).setBold(true).setFontSize(16));//4400
		if (trans.hgvs_c.length() == 0 && trans.hgvs_p.length() == 0)
		{
			row.addCell(1900, QByteArrayList() << RtfText("???").setFontSize(16).highlight(3).RtfCode() << RtfText(trans.id).setFontSize(14).RtfCode());
		}
		else
		{
			row.addCell(1900, QByteArrayList() << RtfText(trans.hgvs_c + ", " + trans.hgvs_p).setFontSize(16).RtfCode() << RtfText(trans.id).setFontSize(14).RtfCode());
		}

		row.addCell(1300, trans.type.replace("_variant",""), RtfParagraph().setFontSize(16));
		row.addCell(700, formatDigits(var.annotations()[i_tum_af].toDouble(), 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

		//RNA data
		if (var_details.depth > 4)
		{
			row.addCell( 700, formatDigits(var_details.frequency, 2), RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		}
		else
		{
			row.addCell( 700, "n/a", RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		}

		row.addCell( 1200, formatDigits(data.tumor_tpm) , RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		row.addCell( 1200, BasicStatistics::isValidFloat(data.hpa_ref_tpm) ? formatDigits(data.hpa_ref_tpm) : "-", RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );
		row.addCell( 1000, formatDigits(data.cohort_mean_tpm), RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );

		row.addCell( 1121, expressionChange(data) , RtfParagraph().setHorizontalAlignment("c").setFontSize(16) );

		for(int i=4; i<row.count(); ++i) row[i].setBackgroundColor(4);
		table.addRow(row);
	}
	table.sortByCol(0);

	RtfTableRow header = RtfTableRow({"Gen", "Veränderung", "Typ", "Anteil", "Anteil", "Tumorprobe TPM", "Normalprobe TPM", "Tumortyp\n\\line\nMW-TPM", "Veränderung\n\\line\n(x-fach)"},{800,1900,1300,700,700,1200,1200, 1000, 1121}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader().setBorders(1, "brdrhair", 2);
	for(int i=4; i<header.count(); ++i) header[i].setBackgroundColor(4);
	table.prependRow( header );
	RtfTableRow sub_header = RtfTableRow({"DNA", "RNA"}, {4700, 5221}, RtfParagraph().setFontSize(16).setHorizontalAlignment("c").setBold(true)).setBorders(1, "brdrhair", 2);
	sub_header[1].setBackgroundColor(4);
	table.prependRow(sub_header);
	table.prependRow(RtfTableRow("Punktmutationen (SNVs) und kleine Insertionen/Deletionen (INDELs) mit unklarer Onkogenität", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.setUniqueBorder(1, "brdrhair", 2);

	return table;
}

RtfSourceCode SomaticRnaReport::trans(QString orig_entry, int font_size) const
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
		dict["translocation"] = "Translokation";
		dict["translocation/5'-5'"] = "Translokation/5'-5'";
		dict["inversion"] = "Inversion";
		dict["inversion/3'-3'"] = "Inversion/3'-3'";
		dict["inversion/5'-5'"] = "Inversion/5'-5'";
		dict["duplication"] = "Duplikation";
		dict["duplication/5'-5'"] = "Duplikation/3'-3'";
		dict["duplication/5'-5'"] = "Duplikation/5'-5'";
		dict["deletion/read-through"] = "Deletion/Read-through";
		dict["deletion/read-through/3'-3'"] = "Deletion/Read-through/3'-3'";
		dict["deletion"] = "Deletion";
		dict["FGFR signaling pathway"] = "FGFR Signalweg";
		dict["immune response"] = "Immunantwort";
		dict["promoter activity"] = "Promotoraktivität";
		dict["RAS signaling pathway"] = "RAS Signalweg";
		dict["RTK signaling pathway"] = "RTK Signalweg";
		dict["TNF signaling pathway"] = "TNF Signalweg";
		dict["DNA repair"] = "DNA-Reparatur";
		dict["DNA replication"] = "DNA-Replikation";
		dict["epigenetics"] = "Epigenetik";
		dict["CDK4/6 signaling pathway"] = "CDK4/6 Signalweg";
		dict["mTOR signaling pathway"] = "mTOR Signalweg";
	}

	if(!dict.contains(orig_entry)) //Return highlighted original entry if not found
	{
		if (font_size != -1)
		{
			return RtfText(orig_entry.toUtf8()).setFontSize(font_size).highlight(3).setItalic(true).RtfCode();
		}
		return RtfText(orig_entry.toUtf8()).highlight(3).setItalic(true).RtfCode();
	}

	return dict[orig_entry];
}

void SomaticRnaReport::writeRtf(QByteArray out_file)
{
	doc_.setDefaultFontSize(16);

	doc_.addColor(191,191,191);
	doc_.addColor(161,161,161);
	doc_.addColor(255,255,0);
	doc_.addColor(242, 242, 242);
	doc_.addColor(255,0,0);

	if(dna_snvs_.count() > 0)
	{
		doc_.addPart(RtfParagraph("Potentiell relevante somatische Veränderungen:").setFontSize(18).setBold(true).RtfCode());
		doc_.addPart(partSnvTable().RtfCode());
	}
	else doc_.addPart(RtfParagraph("Es wurden keine SNVs detektiert.").RtfCode());


	doc_.addPart(RtfParagraph("").RtfCode());

	if(dna_cnvs_.count() > 0)
	{
		doc_.addPart(partCnvTable().RtfCode());
	}

	doc_.addPart(partVarExplanation().RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());

	doc_.newPage();


	if(svs_.count() > 0)
	{
		doc_.addPart(RtfParagraph("Strukturvarianten:").setFontSize(18).setBold(true).RtfCode());
		RtfSourceCode tmp_text = "Es wurden keine therapierelevanten Fusionen nachgewiesen.";
		doc_.addPart(RtfParagraph(tmp_text).setHorizontalAlignment("j").setFontSize(16).highlight(3).RtfCode());
		tmp_text = "Die Sequenzierung der Tumor-DNA gibt Hinweise auf die aktive Form EGFRvIII. Die Transkriptomdaten zeigen eine Überexpression von EGFR, bestätigen jedoch nicht die Deletion der Exons 2-7 der Form EGFRvIII. ";
		tmp_text += "Die in der DNA-Sequenzierung gefundene Fusion konnte bestätigt werden.";
		doc_.addPart(RtfParagraph(tmp_text).setHorizontalAlignment("j").setFontSize(16).highlight(3).RtfCode());

		tmp_text = "Es wurde eine möglicherweise therapierelevante Fusion zwischen ";
		tmp_text += RtfText("XXXX").highlight(3).setFontSize(16).RtfCode() + " und " + RtfText("XXXX").highlight(3).setFontSize(16).RtfCode() + " nachgewiesen (s. Abbildung unten). ";
		tmp_text += "Es wurde eine möglicherweise therapierelevante Deletion innerhalb von ";
		tmp_text += RtfText("XXXX").highlight(3).setFontSize(16).RtfCode() + " nachgewiesen, die zu einem möglichen Verlust der Splice-Region nach " + RtfText("Exon X").highlight(3).setFontSize(16).RtfCode() + " führen könnte (s. Abbildung unten).";
		doc_.addPart(RtfParagraph(tmp_text).setFontSize(16).RtfCode());

	}

	if(svs_.count() > 0)
	{
		doc_.addPart(partFusions().RtfCode());
		doc_.addPart(RtfParagraph("").RtfCode());
	}
	else doc_.addPart(RtfParagraph("Es wurden keine Strukturvarianten detektiert.").RtfCode());

	if(svs_.count() > 0)
	{
		doc_.addPart(partSVs().RtfCode());
		doc_.addPart(RtfParagraph("").RtfCode());
	}
	else doc_.addPart(RtfParagraph("Es wurden keine Fusionen detektiert.").RtfCode());

	if (data_.fusion_pics.count() > 0)
	{
		//Description of fusion pics.
		RtfSourceCode desc = "Gezeigt wird die mögliche Strukturvariante als lineare und als kreisförmige Genom-Darstellung, ";
		desc += "beteiligte Chromosomen, Fusionspartner, ihre Orientierung, Bruchpunkte im Genom, beteiligte Exons und ihre Abdeckung aus den Sequenzierdaten, das Fusionstranskript, ";
		desc += "Anzahl der für die Detektion unterstützenden Reads und Funktionelle Domäne im Protein mit der Position der Exons. ";
		desc += "Für Strukturvarianten innerhalb eines Gens wird zu Visualisierungszwecken eine Kopie des Gens zusätzlich dargestellt.";
		doc_.addPart(RtfParagraph(desc).setFontSize(16).setHorizontalAlignment("j").RtfCode());
	}

	if(data_.expression_plots.count() > 0)
	{
		doc_.newPage();
		doc_.addPart(partExpressionPics());
		doc_.newPage();
		doc_.addPart(RtfParagraph("").RtfCode());
		doc_.newPage();
	}

	doc_.addPart(partGeneExpression().RtfCode());
	doc_.addPart(partGeneExprExplanation().RtfCode());
	doc_.newPage();

	doc_.addPart(partTop10Expression());
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.newPage();

	doc_.addPart(RtfParagraph("Expression der Gene mit unklaren Varianten").setFontSize(18).setBold(true).RtfCode());
	doc_.addPart(uncertainSnvTable().RtfCode());
	doc_.addPart(partVarExplanation().RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.newPage();

	doc_.addPart(partGeneralInfo().RtfCode());

	if(data_.fusion_pics.count() > 0)
	{
		doc_.newPage();
		doc_.addPart(partFusionPics());
		doc_.addPart(RtfParagraph("").RtfCode());
	}

	doc_.save(out_file);
}

RtfSourceCode SomaticRnaReport::formatDigits(double in, int digits)
{
	if(!BasicStatistics::isValidFloat(in)) return "n/a";

	return QByteArray::number(in, 'f', digits);
}

RtfSourceCode SomaticRnaReport::expressionChange(const ExpressionData& data)
{
	RtfSourceCode out = "-";
	if(data.pvalue < 0.05) out = formatDigits(std::pow(2., data.log2fc), 1) + "\\super*";
	else if(data.tumor_tpm > 10 && data_.cohort_size > 5) out =  formatDigits(std::pow(2., data.log2fc), 1);

	return out;
}
