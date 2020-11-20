#include <QDebug>
#include "NGSD.h"
#include "SomaticRnaReport.h"
#include "SomaticReportHelper.h"
#include "TSVFileStream.h"

SomaticRnaReport::SomaticRnaReport(const VariantList& snv_list, const FilterCascade& filters, const CnvList& cnv_list, QString rna_ps_name)
	: db_()
	, dna_cnvs_(cnv_list)
{
	dna_snvs_.copyMetaData(snv_list);
	QBitArray som_filters_pass = filters.apply(snv_list).flags();
	for(int i=0; i<snv_list.count(); ++i)
	{
		if(!som_filters_pass[i]) continue;
		dna_snvs_.append(snv_list[i]);
	}
	rna_ps_name_ = rna_ps_name;

	//Get RNA processed sample name and resolve path to RNA directory
	QString rna_sample_dir ="";

	ref_tissue_type_ = refTissueType(dna_snvs_);

	rna_sample_dir = db_.processedSamplePath(db_.processedSampleId(rna_ps_name_), NGSD::PathType::SAMPLE_FOLDER);


	QStringList fusion_files = Helper::findFiles(rna_sample_dir, "*_var_fusions.tsv", false);
	if(fusion_files.count() > 1)
	{
		THROW(FileAccessException, "Found more than 1 file with RNA fusions in " + rna_sample_dir);
	}
	if(fusion_files.count() == 0)
	{
		THROW(FileAccessException, "Could not find any files with RNA fusions in " + rna_sample_dir);
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
	TSVFileStream in_file(fusion_files[0]);
	int i_cds_left = in_file.colIndex("CDS_LEFT_ID", true);
	int i_cds_left_range = in_file.colIndex("CDS_LEFT_RANGE", true);
	int i_cds_right = in_file.colIndex("CDS_RIGHT_ID", true);
	int i_cds_right_range = in_file.colIndex("CDS_RIGHT_RANGE", true);
	int i_fusion_name = in_file.colIndex("FusionName", true);
	int i_fusion_type = in_file.colIndex("PROT_FUSION_TYPE", true);

	while(!in_file.atEnd())
	{
		QByteArrayList parts = in_file.readLine();

		if(parts[i_cds_left].trimmed() == "." || parts[i_cds_left].isEmpty()) continue;

		fusion temp{parts[i_fusion_name].replace("--","-"), parts[i_cds_left], parts[i_cds_right], parts[i_cds_left_range], parts[i_cds_right_range], parts[i_fusion_type]};
		fusions_.append(temp);
	}

}

bool SomaticRnaReport::checkRequiredSNVAnnotations(const VariantList& variants)
{
	//neccessary DNA annotations (exact match)
	const QByteArrayList an_names_dna = {"coding_and_splicing", "tumor_af", "CGI_driver_statement", "somatic_classification", "ncg_tsg", "ncg_oncogene"};
	for(QByteArray an : an_names_dna)
	{
		if(variants.annotationIndexByName(an, true, false) == -1) return false;
	}
	//neccessary RNA annotation (from RNA samples), (no exact match, because multiple RNA annotations possible)
	const QByteArrayList an_names_rna = {"_rna_depth", "_rna_af", "_rna_tpm", "rna_ref_tpm"};
	for(QByteArray an : an_names_rna)
	{
		if(variants.annotationIndexByName(an,false, false) == -1) return false;
	}

	return true;
}

bool SomaticRnaReport::checkRequiredCNVAnnotations(const CnvList &cnvs)
{
	QByteArrayList an_names_dna = {"cnv_type", "CGI_genes", "CGI_driver_statement", "CGI_gene_role", "ncg_oncogene", "ncg_tsg"};
	for(QByteArray an : an_names_dna)
	{
		if(cnvs.annotationIndexByName(an, false) < 0) return false;
	}

	QByteArrayList an_names_rna = {"_rna_tpm", "rna_ref_tpm"};
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

int SomaticRnaReport::rankCnv(double tpm, double mean_tpm, GeneRole gene_role, bool oncogene, bool tsg)
{
	double ratio = tpm/mean_tpm;

	if(gene_role == GeneRole::LOF && ratio <= 0.8)
	{
		return 1;
	}
	else if(gene_role == GeneRole::ACTIVATING && ratio >= 1.5)
	{
		return 1;
	}
	else //fallback to TSG/Oncogene
	{
		if(tsg && ratio <= 0.8) return 1;
		else if(oncogene && ratio >= 1.5) return 1;
	}

	return 2;
}


RtfTable SomaticRnaReport::fusions()
{
	RtfTable fusion_table;
	fusion_table.addRow(RtfTableRow("Fusionen", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1));

	fusion_table.addRow(RtfTableRow({"Fusion", "Transkript links", "Aminosäure linkes Gen", "Transkript rechts", "Aminosäure rechtes Gen", "Fusionstyp"},{2000,1800,1200,1800,1200,1921}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());
	for(const fusion& fus : fusions_)
	{
		//fus.genes is parsed because nomenclature for fusions contains only one dash instead of 2 dashes
		fusion_table.addRow(RtfTableRow({fus.genes, fus.transcipt_id_left, fus.aa_left, fus.transcipt_id_right, fus.aa_right, fus.type},{2000,1800,1200,1800,1200,1921}, RtfParagraph().setFontSize(16)));
	}

	fusion_table.setUniqueBorder(1,"brdrhair",2);

	fusion_table.addRow(RtfTableRow("Fusionen vom Typ Frameshift führen wahrscheinlich zu einem nicht-funktionellen Fusionsprotein.",doc_.maxWidth(), RtfParagraph().setFontSize(14)));

	return fusion_table;
}


RtfTable SomaticRnaReport::snvTable()
{
	RtfTable table;

	int i_co_sp = dna_snvs_.annotationIndexByName("coding_and_splicing");
	int i_tum_af = dna_snvs_.annotationIndexByName("tumor_af");
	int i_cgi_statem = dna_snvs_.annotationIndexByName("CGI_driver_statement");

	int i_rna_tpm = dna_snvs_.annotationIndexByName(rna_ps_name_ + "_rna_tpm");
	int i_rna_af = dna_snvs_.annotationIndexByName(rna_ps_name_ + "_rna_af");

	int i_rna_ref_tpm = dna_snvs_.annotationIndexByName("rna_ref_tpm");

	int i_som_class = dna_snvs_.annotationIndexByName("somatic_classification");
	int i_tsg = dna_snvs_.annotationIndexByName("ncg_tsg");
	int i_oncogene = dna_snvs_.annotationIndexByName("ncg_oncogene");


	for(int i=0; i<dna_snvs_.count(); ++i)
	{
		const Variant& var = dna_snvs_[i];
		if(!var.annotations()[i_cgi_statem].contains("known") && !var.annotations()[i_cgi_statem].contains("driver") && !var.annotations()[i_som_class].contains("activating")) continue;

		VariantTranscript trans = var.transcriptAnnotations(i_co_sp)[0];

		RtfTableRow row;

		row.addCell(1000, trans.gene, RtfParagraph().setItalic(true).setBold(true).setFontSize(16));
		row.addCell({RtfText(trans.hgvs_c + ":" + trans.hgvs_p).setFontSize(16).RtfCode(), RtfText(trans.id).setFontSize(14).RtfCode()}, 2550);
		row.addCell(1150, trans.type.replace("_variant",""), RtfParagraph().setFontSize(16));

		row.addCell(750, QByteArray::number(var.annotations()[i_tum_af].toDouble(), 'f', 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));



		RtfSourceCode description = "unklare Funktion";
		if(!var.annotations()[i_som_class].isEmpty())
		{
			description = SomaticRnaReport::trans( QString(var.annotations()[i_som_class]) );
		}
		else if(var.annotations()[i_cgi_statem].contains("known"))
		{
			description = "Treiber (bekannt)";
		}
		else if(var.annotations()[i_cgi_statem].contains("predicted driver"))
		{
			description = "Treiber (vorhergesagt)";
		}

		if(var.annotations().at(i_oncogene).contains("1") ) description.append(", Onkogen");
		if(var.annotations().at(i_tsg).contains("1") ) description.append(", TSG");


		row.addCell(1950, description, RtfParagraph().setFontSize(16));

		row.addCell(675, var.annotations()[i_rna_af], RtfParagraph().setHorizontalAlignment("c").setFontSize(16));

		bool ok = false;
		QByteArray tpm_formatted = QByteArray::number(var.annotations()[i_rna_tpm].toDouble(&ok), 'f', 1);
		if(!ok) tpm_formatted = ".";

		row.addCell(625, tpm_formatted , RtfParagraph().setHorizontalAlignment("c").setFontSize(16));

		ok = false;
		QByteArray ref_tpm_formatted = QByteArray::number(var.annotations()[i_rna_ref_tpm].toDouble(&ok), 'f', 1);
		if(!ok) ref_tpm_formatted = ".";
		row.addCell(1221, ref_tpm_formatted, RtfParagraph().setHorizontalAlignment("c").setFontSize(16));

		table.addRow(row);
	}
	table.sortByCol(0);


	table.prependRow(RtfTableRow({"Gen", "Veränderung", "Typ", "Anteil", "Beschreibung", "Anteil", "TPM", "MW TPM*"},{1000,2550,1150,750,1950,675,625,1221}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader().setBorders(1, "brdrhair", 2) );
	table.prependRow(RtfTableRow({"DNA", "RNA"}, {7400, 2521}, RtfParagraph().setFontSize(16).setHorizontalAlignment("c").setBold(true)).setBorders(1, "brdrhair", 2) );
	table.prependRow(RtfTableRow("Punktmutationen (SNVs) und kleine Insertionen/Deletionen (INDELs)", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.setUniqueBorder(1, "brdrhair", 2);

	RtfSourceCode desc = "";

	desc += RtfText("Anteil:").setBold(true).setFontSize(14).RtfCode() + " " + "Anteil der Allele mit der gelisteten Variante (SNV, INDEL). ";
	desc += RtfText("Beschreibung:").setBold(true).setFontSize(14).RtfCode() + " " + "Klassifikation der Varianten und ggf. Bewertung der Genfunktion als Onkogen bzw. Tumorsuppressorgen (TSG). ";
	desc += RtfText("*MW TPM:").setBold(true).setFontSize(14).RtfCode() + " " + "Mittelwerte in Transcripts per Million (TPM) von Vergleichsproben aus " + trans(ref_tissue_type_) + " (The Human Protein Atlas). ";
	desc += RtfText("Erweiterte Legende und Abkürzungen siehe Anlage 1.").setFontSize(14).RtfCode();
	table.addRow(RtfTableRow(desc, 9921, RtfParagraph().setFontSize(14)));

	return table;
}

RtfTable SomaticRnaReport::cnvTable()
{
	//DNA annotation indices
	int i_cgi_genes = dna_cnvs_.annotationIndexByName("CGI_genes", false);
	int i_cgi_driver_statement = dna_cnvs_.annotationIndexByName("CGI_driver_statement", false);
	int i_cgi_gene_role = dna_cnvs_.annotationIndexByName("CGI_gene_role", false);

	int i_tumor_cn = dna_cnvs_.annotationIndexByName("tumor_CN_change", true);
	int i_tum_clonality = dna_cnvs_.annotationIndexByName("tumor_clonality", true);
	int i_size_desc = dna_cnvs_.annotationIndexByName("cnv_type", true);
	//RNA annotations indices
	int i_rna_tpm = dna_cnvs_.annotationIndexByName(rna_ps_name_.toUtf8() + "_rna_tpm", true);
	int i_ref_rna_tpm = dna_cnvs_.annotationIndexByName("rna_ref_tpm", true);

	//TSG/Oncogene
	int i_tsg = dna_cnvs_.annotationIndexByName("ncg_tsg", true);
	int i_oncogene = dna_cnvs_.annotationIndexByName("ncg_oncogene", true);


	RtfTable temp_table;
	for(int i=0; i<dna_cnvs_.count(); ++i)
	{
		const CopyNumberVariant& cnv = dna_cnvs_[i];

		QByteArrayList genes = dna_cnvs_[i].annotations()[i_cgi_genes].split(',');
		QByteArrayList statements = dna_cnvs_[i].annotations()[i_cgi_driver_statement].split(',');
		QByteArrayList cgi_gene_roles = dna_cnvs_[i].annotations()[i_cgi_gene_role].split(',');
		QByteArrayList tumor_sup_genes = dna_cnvs_[i].annotations()[i_tsg].split(',');
		QByteArrayList oncogenes = dna_cnvs_[i].annotations()[i_oncogene].split(',');

		for(int j=0; j<statements.count(); ++j)
		{
			if(statements[j].contains("known") || statements[j].contains("driver"))
			{
				RtfTableRow temp;
				temp.addCell(1000, genes[j], RtfParagraph().setBold(true).setItalic(true).setFontSize(16));
				temp.addCell(2100, cnv.chr().str() + " (" + cnv.annotations().at(i_size_desc).trimmed() + ")", RtfParagraph().setFontSize(16));

				bool ok = false;
				int tumor_cn = cnv.annotations().at(i_tumor_cn).toInt(&ok);
				if(!ok) tumor_cn = -1;
				temp.addCell(1300, SomaticReportHelper::CnvTypeDescription(tumor_cn), RtfParagraph().setFontSize(16));


				temp.addCell(500, cnv.annotations().at(i_tumor_cn), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

				temp.addCell(750, QByteArray::number(cnv.annotations().at(i_tum_clonality).toDouble(), 'f', 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));



				bool is_oncogene = false, is_tsg = false;
				if(tumor_sup_genes[j].contains("1")) is_tsg = true;
				else if(oncogenes[j].contains("1")) is_oncogene = true;


				RtfSourceCode description = "";
				if(is_oncogene) description += ", Onkogen";
				if(is_tsg) description += ", TSG";
				if(!is_oncogene && !is_tsg) description += ", NA";
				description = description.mid(2); //remove leading comma

				temp.addCell(1886, description , RtfParagraph().setFontSize(16));

				temp.addCell(900, QByteArray::number(getTpm(genes[j], cnv.annotations().at(i_rna_tpm)), 'f', 1), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
				temp.addCell(900, QByteArray::number(getTpm(genes[j], cnv.annotations().at(i_ref_rna_tpm)), 'f', 1), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );


				//Determine oncogenetic rank of expression change
				GeneRole gene_role = GeneRole::AMBIGOUS;
				if(cgi_gene_roles[j].contains("LoF")) gene_role = GeneRole::LOF;
				else if(cgi_gene_roles[j].contains("Act")) gene_role = GeneRole::ACTIVATING;
				int rank = rankCnv( getTpm(genes[j], cnv.annotations().at(i_rna_tpm)) , getTpm(genes[j], cnv.annotations().at(i_ref_rna_tpm)), gene_role , is_oncogene , is_tsg);

				temp.addCell( 585 , QByteArray::number(rank) , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
				temp_table.addRow(temp);
			}
		}
	}

	temp_table.sortByCol(0); //sort by gene name alphabetically


	RtfTable table;

	//Add rows to output table, sorted by rank
	for(int i=0; i<temp_table.count(); ++i)
	{
		if(temp_table[i][8].format().content().contains("1")) table.addRow(temp_table[i]);
	}
	for(int i=0; i<temp_table.count(); ++i)
	{
		if(temp_table[i][8].format().content().contains("2")) table.addRow(temp_table[i]);
	}

	table.prependRow(RtfTableRow({"Gen", "Position", "CNV", "CN", "Anteil","Beschreibung","TPM RNA", "MW TPM*", "Rang"},{1000,2100,1300,500,750,1886,900,900,585}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader());
	table.prependRow(RtfTableRow("Kopienzahlveränderungen (CNVs)", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.setUniqueBorder(1, "brdrhair", 2);

	RtfSourceCode desc = "";
	desc += RtfText("*MW TPM:").setBold(true).setFontSize(14).RtfCode() + " " + "Mittelwerte von Vergleichsproben aus " + trans(ref_tissue_type_) + " (The Human Protein Atlas). ";
	desc += RtfText("Rang 1:").setBold(true).setFontSize(14).RtfCode() + " Die Expression eines Onkogens ist in der Probe erhöht oder die Expression eines Tumor-Suppresor-Gens ist in der Probe reduziert. ";
	desc += RtfText("Rang 2:").setBold(true).setFontSize(14).RtfCode() + " Die Expression ist in der Probe und in der Kontrolle ähnlich oder die Rolle des Gens in der Onkogenese ist nicht eindeutig.";
	table.addRow(RtfTableRow(desc, 9921, RtfParagraph().setFontSize(14)));

	return table;
}

double SomaticRnaReport::getTpm(QByteArray gene, QByteArray field)
{
	QByteArrayList entries = field.split(',');
	for(QByteArray& entry : entries)
	{
		QList<QByteArray> parts = entry.append('=').split('=');

		if(parts[0] == gene)
		{
			bool ok = false;
			double res = parts[1].toDouble(&ok);

			if(ok) return res;
		}
	}

	return -1.;
}

RtfSourceCode SomaticRnaReport::trans(QString orig_entry) const
{
	QHash<QString, RtfSourceCode> dict;

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

	if(dna_snvs_.count() > 0) doc_.addPart(snvTable().RtfCode());
	else doc_.addPart(RtfParagraph("Es wurden keine SNVs detektiert.").RtfCode());


	doc_.addPart(RtfParagraph("").RtfCode());

	if(dna_cnvs_.count() > 0) doc_.addPart(cnvTable().RtfCode());
	else doc_.addPart(RtfParagraph("Es wurden keine CNVs detektiert.").RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());

	if(fusions_.count() > 0) doc_.addPart(fusions().RtfCode());
	else doc_.addPart(RtfParagraph("Es wurden keine proteinkodierenden Fusionen gefunden.").RtfCode());


	doc_.save(out_file);
}
