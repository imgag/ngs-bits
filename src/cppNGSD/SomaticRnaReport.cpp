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
	TSVFileStream in_file(data.rna_fusion_file);
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
	const QByteArrayList an_names_dna = {"coding_and_splicing", "tumor_af", "ncg_tsg", "ncg_oncogene"};
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
	QByteArrayList an_names_dna = {"cnv_type", "ncg_oncogene", "ncg_tsg"};
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

int SomaticRnaReport::rankCnv(double tpm, double mean_tpm, SomaticGeneRole::Role gene_role, bool oncogene, bool tsg)
{
	double ratio = tpm/mean_tpm;

	if(gene_role == SomaticGeneRole::Role::LOSS_OF_FUNCTION && ratio <= 0.8)
	{
		return 1;
	}
	else if(gene_role == SomaticGeneRole::Role::ACTIVATING && ratio >= 1.5)
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

	int i_rna_tpm = dna_snvs_.annotationIndexByName(data_.rna_ps_name + "_rna_tpm");
	int i_rna_af = dna_snvs_.annotationIndexByName(data_.rna_ps_name + "_rna_af");
	int i_rna_ref_tpm = dna_snvs_.annotationIndexByName("rna_ref_tpm");

	int i_tsg = dna_snvs_.annotationIndexByName("ncg_tsg");
	int i_oncogene = dna_snvs_.annotationIndexByName("ncg_oncogene");


	for(int i=0; i<dna_snvs_.count(); ++i)
	{
		const Variant& var = dna_snvs_[i];


		if(db_.getSomaticViccId(var) == -1) continue;
		SomaticViccData vicc_data = db_.getSomaticViccData(var);
		SomaticVariantInterpreter::Result vicc_result = SomaticVariantInterpreter::viccScore(vicc_data);
		if(vicc_result != SomaticVariantInterpreter::Result::ONCOGENIC && vicc_result != SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC) continue;

		VariantTranscript trans = var.transcriptAnnotations(i_co_sp)[0];

		RtfTableRow row;

		row.addCell(1000, trans.gene, RtfParagraph().setItalic(true).setBold(true).setFontSize(16));
		row.addCell({RtfText(trans.hgvs_c + ":" + trans.hgvs_p).setFontSize(16).RtfCode(), RtfText(trans.id).setFontSize(14).RtfCode()}, 2550);
		row.addCell(1150, trans.type.replace("_variant",""), RtfParagraph().setFontSize(16));
		row.addCell(750, QByteArray::number(var.annotations()[i_tum_af].toDouble(), 'f', 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

		RtfSourceCode description = SomaticReportHelper::trans(SomaticVariantInterpreter::viccScoreAsString(vicc_data)).toUtf8();

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
	table.prependRow(RtfTableRow("Punktmutationen (SNVs) und kleine Insertionen/Deletionen (INDELs) (" + data_.rna_ps_name.toUtf8() + "-" + data_.tumor_ps.toUtf8() + "-" + data_.normal_ps.toUtf8() + ")", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

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
	int i_tum_clonality = dna_cnvs_.annotationIndexByName("tumor_clonality", true);
	int i_size_desc = dna_cnvs_.annotationIndexByName("cnv_type", true);
	//RNA annotations indices
	int i_rna_tpm = dna_cnvs_.annotationIndexByName(data_.rna_ps_name.toUtf8() + "_rna_tpm", true);
	int i_ref_rna_tpm = dna_cnvs_.annotationIndexByName("rna_ref_tpm", true);

	//TSG/Oncogene
	int i_tsg = dna_cnvs_.annotationIndexByName("ncg_tsg", true);
	int i_oncogene = dna_cnvs_.annotationIndexByName("ncg_oncogene", true);


	RtfTable table;
	for(int i=0; i<dna_cnvs_.count(); ++i)
	{
		const CopyNumberVariant& cnv = dna_cnvs_[i];

		GeneSet genes = dna_cnvs_[i].genes().intersect(data_.processing_system_genes);

		GeneSet tsgs = GeneSet::createFromText(dna_cnvs_[i].annotations()[i_tsg], ',' );
		GeneSet oncogenes = GeneSet::createFromText( dna_cnvs_[i].annotations()[i_oncogene] , ',' );

		for(const auto& gene : genes)
		{
			if(db_.getSomaticGeneRoleId(gene) == -1 ) continue;
			SomaticGeneRole role = db_.getSomaticGeneRole(gene, true);

			if( !SomaticCnvInterpreter::includeInReport(dna_cnvs_,cnv, role) ) continue;
			if( !role.high_evidence) continue;


			RtfTableRow temp;
			temp.addCell(1000, gene, RtfParagraph().setBold(true).setItalic(true).setFontSize(16));
			temp.addCell(2100, cnv.chr().str() + " (" + cnv.annotations().at(i_size_desc).trimmed() + ")", RtfParagraph().setFontSize(16));

			int tumor_cn = cnv.copyNumber(dna_cnvs_.annotationHeaders());
			temp.addCell(1300, SomaticReportHelper::CnvTypeDescription(tumor_cn), RtfParagraph().setFontSize(16));


			temp.addCell(500, QByteArray::number(tumor_cn), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

			temp.addCell(750, QByteArray::number(cnv.annotations().at(i_tum_clonality).toDouble(), 'f', 2), RtfParagraph().setFontSize(16).setHorizontalAlignment("c"));

			bool is_oncogene = false, is_tsg = false;
			if(tsgs.contains(gene)) is_tsg = true;
			else if(oncogenes.contains(gene)) is_oncogene = true;


			QList<RtfSourceCode> description;
			if(is_oncogene) description << "Onkogen";
			if(is_tsg) description <<  "TSG";
			if(!is_oncogene && !is_tsg) description << "NA";

			temp.addCell(1886, description.join(", ") , RtfParagraph().setFontSize(16));

			temp.addCell(900, QByteArray::number(getTpm(gene, cnv.annotations().at(i_rna_tpm)), 'f', 1), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
			temp.addCell(900, QByteArray::number(getTpm(gene, cnv.annotations().at(i_ref_rna_tpm)), 'f', 1), RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );


			//Determine oncogenetic rank of expression change
			int rank = rankCnv( getTpm(gene, cnv.annotations().at(i_rna_tpm)) , getTpm(gene, cnv.annotations().at(i_ref_rna_tpm)), role.role , is_oncogene , is_tsg);

			temp.addCell( 585 , QByteArray::number(rank) , RtfParagraph().setFontSize(16).setHorizontalAlignment("c") );
			table.addRow(temp);
		}
	}

	table.sortbyCols({8,0}); //sort by rank tean by gene symbol

	//add table headings
	table.prependRow(RtfTableRow({"Gen", "Position", "CNV", "CN", "Anteil","Beschreibung","TPM RNA", "MW TPM*", "Rang"},{1000,2100,1300,500,750,1886,900,900,585}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader());
	table.prependRow(RtfTableRow({"DNA", "RNA"},{7536,2385}, RtfParagraph().setFontSize(16).setBold(true).setHorizontalAlignment("c")).setHeader() );
	table.prependRow(RtfTableRow("Kopienzahlveränderungen (CNVs)", doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(16)).setHeader().setBackgroundColor(1).setBorders(1, "brdrhair", 2) );

	table.setUniqueBorder(1, "brdrhair", 2);

	//description hints below table
	RtfSourceCode desc = "";
	desc += RtfText("*MW TPM:").setBold(true).setFontSize(14).RtfCode() + " " + "Mittelwerte von Vergleichsproben aus " + trans(ref_tissue_type_) + " (The Human Protein Atlas). ";
	desc += RtfText("Rang 1:").setBold(true).setFontSize(14).RtfCode() + " Die Expression eines Gens mit beschriebenem Funktionsgewinn (Gain of Function) ist in der Probe erhöht oder die Expression eines Gens mit Funktionsverlust (LoF) ist in der Probe reduziert. ";
	desc += RtfText("Rang 2:").setBold(true).setFontSize(14).RtfCode() + " Die Expression ist in der Probe und in der Kontrolle ähnlich oder die Rolle des Gens in der Onkogenese ist nicht eindeutig. ";
	desc += RtfText("NA:").setBold(true).setFontSize(14).RtfCode() + " Keine abschließende Bewertung als Tumor Suppressor Gen (TSG) oder als Onkogen (NCG6.0).";
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
