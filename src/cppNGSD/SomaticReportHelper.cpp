#include "SomaticReportHelper.h"
#include "BasicStatistics.h"
#include "OntologyTermCollection.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "NGSHelper.h"
#include "SomaticReportSettings.h"
#include "NGSD.h"
#include "XmlHelper.h"
#include "GenLabDB.h"
#include "SomaticXmlReportGenerator.h"
#include "SomaticVariantInterpreter.h"
#include <cmath>
#include <QDir>
#include <QMap>
#include <QPair>
#include <QCollator>

bool SomaticReportHelper::checkGermlineSNVFile(const VariantList &germline_variants)
{
	if(germline_variants.count() == 0) return false;

	const QByteArrayList ans = {"gene", "coding_and_splicing", "classification", "dbSNP"};

	for(const auto& an : ans)
	{
		if(germline_variants.annotationIndexByName(an, true, false) < 0) return false;
	}

	return true;
}


RtfTable SomaticReportHelper::cnvTable()
{
	RtfTable cnv_table;

	//Table Header
	cnv_table.addRow(RtfTableRow({"Chromosomale Aberrationen"},doc_.maxWidth(),RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(18)).setBackgroundColor(4).setHeader());
	cnv_table.addRow(RtfTableRow({"Position","CNV","Typ","CN","Anteil","Gene"},{1700,1000,800,400,800,5221},RtfParagraph().setHorizontalAlignment("c").setFontSize(16).setBold(true)).setHeader());

	RtfParagraph header_format;
	header_format.setBold(true);
	header_format.setHorizontalAlignment("c");

	if(cnvs_.isEmpty())
	{
		cnv_table.removeRow(1);
		cnv_table.addRow(RtfTableRow("Es wurden keine CNVs gefunden.",doc_.maxWidth()));
		cnv_table.setUniqueBorder(1,"brdrhair",4);
		return cnv_table;
	}

	if(cnv_index_tumor_clonality_ < 0)
	{
		cnv_table.addRow(RtfTableRow("Die ClinCNV-Datei enthält keine Tumor Clonality. Bitte mit einer aktuelleren Version von ClinCNV neu berechnen.",doc_.maxWidth()));
		cnv_table.setUniqueBorder(1,"brdrhair",4);
		return cnv_table;
	}


	for(int i=0; i<cnvs_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_[i];

		RtfTableRow temp_row;

		//coordinates
		QList<RtfSourceCode> coords;
		coords << RtfText(variant.chr().str()).setFontSize(14).RtfCode();
		coords << RtfText(QByteArray::number(variant.start() == 0 ? 1 : variant.start()) + " - " + QByteArray::number(variant.end())).setFontSize(12).RtfCode();
		temp_row.addCell(coords,1700);


		//AMP/DEL
		temp_row.addCell(1000, CnvTypeDescription(variant.copyNumber(cnvs_.annotationHeaders())), RtfParagraph().setHorizontalAlignment("c").setFontSize(14));

		//Type
		RtfSourceCode type_statement = variant.annotations().at(cnv_index_cnv_type_);
		type_statement = type_statement.replace("chromosome", "chr");
		type_statement = type_statement.replace("partial p-arm", "partial\\line p-arm");
		type_statement = type_statement.replace("partial q-arm", "partial\\line q-arm");
		temp_row.addCell(800, type_statement, RtfParagraph().setHorizontalAlignment("c").setFontSize(14));

		//copy numbers
		temp_row.addCell(400,QByteArray::number(variant.copyNumber(cnvs_.annotationHeaders())), RtfParagraph().setFontSize(14).setHorizontalAlignment("c"));

		//tumor clonality
		temp_row.addCell(800,QByteArray::number(variant.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2).replace(".", ","), RtfParagraph().setHorizontalAlignment("c").setFontSize(14));

		//gene names
		GeneSet genes = settings_.processing_system_genes.intersect( db_.genesToApproved(variant.genes()) );
		std::sort(genes.begin(), genes.end());
		temp_row.addCell(5221,genes.join(", "), RtfParagraph().setItalic(true).setFontSize(14));

		cnv_table.addRow(temp_row);
	}
	cnv_table.setUniqueBorder(1,"brdrhair",4);

	RtfSourceCode desc = RtfText("CNV:").setBold(true).setFontSize(14).RtfCode() + " Kopienzahlvariante, ";
	desc += RtfText("AMP").setBold(true).setFontSize(14).RtfCode() + " Amplifikation, " + RtfText("DEL").setBold(true).setFontSize(14).RtfCode() + " Deletion, ";
	desc += RtfText("LOH").setBold(true).setFontSize(14).RtfCode() + " Kopienzahlneutraler Verlust der Heterozygotie, ";
	desc += RtfText("CN:").setBold(true).setFontSize(14).RtfCode() + " Copy Number, ";
	desc += RtfText("Anteil:").setBold(true).setFontSize(14).RtfCode() + " Anteil der Zellen mit der entsprechenden Kopienzahlvariante in der untersuchten Probe.";

	cnv_table.addRow( RtfTableRow(desc, doc_.maxWidth(), RtfParagraph().setHorizontalAlignment("j").setFontSize(14)) );

	return cnv_table;
}

RtfTable SomaticReportHelper::billingTable()
{
	RtfTable table;

	table.addRow(RtfTableRow({"#Gene", "OMIM"}, {doc_.maxWidth()/2, doc_.maxWidth()/2}, RtfParagraph().setHorizontalAlignment("c").setFontSize(16).setBold(true)).setHeader() );

	BedFile target = settings_.processing_system_roi;
	target.merge();

	BedFile ebm_genes_target = db_.genesToRegions(ebm_genes_,Transcript::SOURCE::ENSEMBL,"gene");
	ebm_genes_target.sort();
	ebm_genes_target.merge();

	ebm_genes_target.intersect(target);

	int size = ebm_genes_target.baseCount();

	if(size < 20000) //fallback to hard coded list of genes in case there are less than 20kb (neccessary for EBM billing)
	{
		ebm_genes_ = GeneSet() << "AKT1" << "ALK" << "APC" << "ARID1A" << "ATM" << "BRAF" << "CCND1" << "CDK4" << "CDKN2A" << "CREBBP" << "CTNNB1" << "DICER1" << "DNMT3A" << "EGFR" << "ERBB2" << "EZH2" << "FGFR1" << "FGFR2" << "FGFR3" << "GNA11" << "GNAQ" << "GNAS" << "IDH1" << "IDH2" << "KIT" << "KRAS" << "MET" << "MTOR" << "MYC" << "MYCN" << "PIK3CA" << "POLE" << "PTEN" << "RAF1" << "SMAD4" << "SMARCA4" << "TGFBR2" << "TP53" << "VHL";
		size = 123670;
	}

	for(const auto& gene : ebm_genes_)
	{
		QByteArrayList omim_mims;
		for(const auto& info :  db_.omimInfo(gene) )
		{
			omim_mims << info.mim;
		}
		table.addRow( RtfTableRow({gene, omim_mims.join(", ")},{doc_.maxWidth()/2, doc_.maxWidth()/2}) );
	}
	table.setUniqueBorder(1);


	table.addRow( RtfTableRow("Basenpaare der abzurechnenden Gene: " + QByteArray::number(size), doc_.maxWidth(), RtfParagraph().setFontSize(14)).setBorders(0) );


	return table;
}


SomaticReportHelper::SomaticReportHelper(const VariantList& variants, const CnvList &cnvs, const VariantList& variants_germline, const SomaticReportSettings& settings)
	: settings_(settings)
	, germline_vl_(variants_germline)
	, cnvs_()
	, validated_viruses_()
	, db_()
{	
	//Assign SNV annotation indices
	snv_index_coding_splicing_ = variants.annotationIndexByName("coding_and_splicing");
	somatic_vl_ = SomaticReportSettings::filterVariants(variants, settings); //filtered out snvs flagged as artefacts

	//Filter CNVs according report configuration settings
	cnvs_ = SomaticReportSettings::filterCnvs(cnvs, settings);

	//load MSI Mantis data
	try
	{
		TSVFileStream msi_file(settings.msi_file);
		//Use step wise difference (-> stored in the first line of MSI status file) for MSI status
		QByteArrayList data = msi_file.readLine();
		if(data.count() > 0) mantis_msi_swd_value_ = data[1].toDouble();
		else mantis_msi_swd_value_ = std::numeric_limits<double>::quiet_NaN();
	}
	catch(...)
	{
		 mantis_msi_swd_value_ =  std::numeric_limits<double>::quiet_NaN();
	}
	if(!settings.report_config.msiStatus()) //if not to be shown in report
	{
		mantis_msi_swd_value_ = std::numeric_limits<double>::quiet_NaN();
	}

	//Load virus data (from tumor sample dir)
	QString viral_file = db_.processedSamplePath(db_.processedSampleId(settings_.tumor_ps), PathType::VIRAL);
	try
	{
		TSVFileStream file(viral_file);
		while(!file.atEnd())
		{
			QByteArrayList parts = file.readLine();
			if(parts.isEmpty()) continue;

			SomaticVirusInfo tmp;
			tmp.chr = parts[0];
			tmp.start = parts[1].toInt();
			tmp.end = parts[2].toInt();
			tmp.name = parts[file.colIndex("name",true)];
			tmp.reads = parts[file.colIndex("reads",true)].toInt();
			tmp.coverage = parts[file.colIndex("coverage",true)].toDouble();
			tmp.mismatches = parts[file.colIndex("mismatches",true)].toInt();
			tmp.idendity = parts[file.colIndex("identity\%",true)].toDouble();

			if(tmp.coverage < 100) continue;
			if(tmp.idendity < 90) continue;

			validated_viruses_ << tmp;
		}
	}
	catch(...) {} //Nothing to do here

	//load obo terms for filtering coding/splicing variants
	OntologyTermCollection obo_terms("://Resources/so-xp_3_0_0.obo",true);
	QList<QByteArray> ids;
	ids << obo_terms.childIDs("SO:0001580",true); //coding variants
	ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
	foreach(const QByteArray& id, ids)
	{
		obo_terms_coding_splicing_.add(obo_terms.getByID(id));
	}



	//assign CNV annotation indices
	cnv_index_cn_change_ = cnvs_.annotationIndexByName("CN_change", false);
	cnv_index_cnv_type_ = cnvs_.annotationIndexByName("cnv_type", false);
	cnv_index_tumor_clonality_ = cnvs_.annotationIndexByName("tumor_clonality", false);
	cnv_index_state_ = cnvs_.annotationIndexByName("state", false);
	cnv_index_cytoband_ = cnvs.annotationIndexByName("cytoband", false);


	//load qcml data
	tumor_qcml_data_ = db_.getQCData(db_.processedSampleId(settings_.tumor_ps));
	normal_qcml_data_ = db_.getQCData(db_.processedSampleId(settings_.normal_ps));

	//load processing system data
	int sys_id = db_.processingSystemIdFromProcessedSample(settings_.tumor_ps);
	processing_system_data_ = db_.getProcessingSystemData(sys_id);

	//load disease details from NGSD
	QStringList tmp;
	QList<SampleDiseaseInfo> disease_info = db_.getSampleDiseaseInfo(db_.sampleId(settings_.tumor_ps));

	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		if(entry.type == "ICD10 code") tmp.append(entry.disease_info);
	}
	icd10_diagnosis_code_ = tmp.join(", ");

	tmp.clear();
	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		if(entry.type == "tumor fraction") tmp.append(entry.disease_info);
	}
	if(tmp.count() == 1) histol_tumor_fraction_ = tmp[0].toDouble();
	else histol_tumor_fraction_ = std::numeric_limits<double>::quiet_NaN();

	tmp.clear();
	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		if(entry.type == "HPO term id") tmp.append(entry.disease_info);
	}
	if(tmp.count() == 1) hpo_term_ = tmp.first();
	else hpo_term_ = "";

	//get mutation burden
	try
	{
		QString mb_string = tumor_qcml_data_.value("QC:2000053",true).toString();
		if (mb_string.contains("var/Mb")) //deal with previous version, e.g. "high (23.79 var/Mb)"
		{
			mb_string = mb_string.append("  ").split(' ')[1].replace("(", "");
		}
		bool ok = false;
		mutation_burden_ = mb_string.toDouble(&ok);
		if(!ok) // deal with 'n/a', '', ...
		{
			mutation_burden_ = std::numeric_limits<double>::quiet_NaN();
		}
	}
	catch(...) //deal with missing QC value
	{
		mutation_burden_ = std::numeric_limits<double>::quiet_NaN();
	}

	//Set up RTF file specifications
	doc_.setMargins( RtfDocument::cm2twip(2.3) , 1134 , RtfDocument::cm2twip(1.2) , 1134 );
	doc_.addColor(188,230,138);
	doc_.addColor(255,0,0);
	doc_.addColor(255,255,0);
	doc_.addColor(191,191,191);
}

void SomaticReportHelper::germlineSnvForQbic(QString path_target_folder)
{
	//currently no germline SNVs are uploaded, only created header
	QSharedPointer<QFile> germline_snvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_germline_snv.tsv");

	QTextStream stream(germline_snvs_qbic.data());

	stream << "chr" << "\t" << "start" << "\t" << "ref" << "\t" << "alt" << "\t" << "genotype" << "\t";
	stream << "gene" << "\t" << "base_change" << "\t" << "aa_change" << "\t" << "transcript" << "\t";
	stream << "functional_class" << "\t" << "effect";
	stream << endl;

	germline_snvs_qbic->close();
}

void SomaticReportHelper::somaticSnvForQbic(QString path_target_folder)
{
	FastaFileIndex genome_reference(Settings::string("reference_genome", false));

	QSharedPointer<QFile> somatic_snvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_somatic_snv.tsv");

	QTextStream stream(somatic_snvs_qbic.data());

	//Write header
	stream << "chr" <<"\t" << "start" << "\t" << "ref" << "\t" << "alt" << "\t";
	stream <<"allele_frequency_tumor" << "\t" << "coverage" << "\t";
	stream << "gene" << "\t" << "base_change" << "\t" << "aa_change" << "\t";
	stream << "transcript" << "\t" << "functional_class" << "\t" << "effect" << endl;

	int i_tumor_af = somatic_vl_.annotationIndexByName("tumor_af",true,true);
	int i_tumor_depth = somatic_vl_.annotationIndexByName("tumor_dp",true,true);

	for(int i=0; i<somatic_vl_.count(); ++i)
	{
		const Variant& variant = somatic_vl_[i];

		VariantVcfRepresentation vcf_rep = variant.toVCF(genome_reference);
		stream << vcf_rep.chr.str() << "\t";
		stream << vcf_rep.pos << "\t";
		stream << vcf_rep.ref << "\t";
		stream << vcf_rep.alt << "\t";
		stream << variant.annotations().at(i_tumor_af) << "\t";
		stream << variant.annotations().at(i_tumor_depth) << "\t";

		//determine transcript, usually first coding/splicing
		VariantTranscript transcript = selectSomaticTranscript(variant);

		//affected gene
		stream << transcript.gene << "\t";

		//base change
		stream << transcript.hgvs_c << "\t";
		//protein change
		stream << transcript.hgvs_p << "\t";
		//transcript
		stream << transcript.id << "\t";
		//functional class
		stream << transcript.type.replace('&',',') << "\t";

		//effect
		QByteArray effect = "NA";
		if(db_.getSomaticViccId(variant) != -1 && db_.getSomaticGeneRoleId(transcript.gene) != -1)
		{
			SomaticVariantInterpreter::Result result = SomaticVariantInterpreter::viccScore(db_.getSomaticViccData(variant));
			SomaticGeneRole gene_role = db_.getSomaticGeneRole(transcript.gene);
			if(gene_role.role == SomaticGeneRole::Role::ACTIVATING && (result == SomaticVariantInterpreter::Result::ONCOGENIC || result == SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC) )
			{
				effect = "activating";
			}
			else if(gene_role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION && (result == SomaticVariantInterpreter::Result::ONCOGENIC || result == SomaticVariantInterpreter::Result::LIKELY_ONCOGENIC) )
			{
				effect = "inactivating";
			}
			else
			{
				effect = "ambiguous";
			}

		}
		else effect = "NA";

		stream << effect;

		stream << endl;
	}
	somatic_snvs_qbic->close();
}

void SomaticReportHelper::germlineCnvForQbic(QString path_target_folder)
{
	QSharedPointer<QFile> germline_cnvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_germline_cnv.tsv");

	QTextStream stream(germline_cnvs_qbic.data());

	stream << "size" << "\t" << "type" << "\t" << "copy_number" << "\t" << "gene" << "\t" << "exons" << "\t" << "transcript" << "\t";
	stream << "chr" << "\t" << "start" << "\t" << "end" << "\t" << "effect";
	stream << endl;

	germline_cnvs_qbic->close();
}


void SomaticReportHelper::somaticCnvForQbic(QString path_target_folder)
{
	QSharedPointer<QFile> somatic_cnvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_somatic_cnv.tsv");

	QTextStream stream(somatic_cnvs_qbic.data());

	stream << "size" << "\t" << "type" << "\t" << "copy_number" << "\t" << "gene" << "\t" << "exons" << "\t";
	stream << "transcript" << "\t" << "chr" << "\t" << "start" << "\t" << "end" << "\t" << "effect" << endl;

	for(int i=0; i < cnvs_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_[i];

		GeneSet genes_in_report = settings_.processing_system_genes.intersect(db_.genesToApproved(variant.genes()) );

		if(cnv_index_cnv_type_ < 0)
		{
			stream << "";
		}
		else
		{
			stream << variant.annotations().at(cnv_index_cnv_type_);
		}
		stream << "\t";

		int copy_number = variant.copyNumber(cnvs_.annotationHeaders());

		if(copy_number > 2)
		{
			stream << "amp";
		}
		else if(copy_number <2)
		{
			stream << "del";
		}
		else
		{
			stream << "loh";
		}

		stream << "\t";
		stream << copy_number << "\t";

		if(genes_in_report.count() > 0)
		{
			for(int j=0;j<genes_in_report.count();++j)
			{
				stream << genes_in_report[j];
				if(j<genes_in_report.count()-1) stream << ";";
			}
		}
		else
		{
			stream << "NA";
		}
		stream << "\t";
		stream << "";
		stream << "\t";
		stream << "";
		stream << "\t";

		stream << variant.chr().str();
		stream << "\t";
		stream << variant.start();
		stream << "\t";
		stream << variant.end();
		stream << "\t";

		//genes in target region
		GeneSet genes = settings_.processing_system_genes.intersect(db_.genesToApproved(variant.genes()) );

		QByteArrayList gene_effects;
		for(const auto& gene : genes)
		{
			if(db_.getSomaticGeneRoleId(gene) == -1) continue;

			SomaticGeneRole::Role role = db_.getSomaticGeneRole(gene).role;

			QByteArray effect = "";

			if(variant.copyNumber(cnvs_.annotationHeaders()) > 2 && role == SomaticGeneRole::Role::ACTIVATING)
			{
				effect = "activating";
			}
			else if(variant.copyNumber(cnvs_.annotationHeaders()) < 2 && role == SomaticGeneRole::Role::LOSS_OF_FUNCTION)
			{
				effect = "inactivating";
			}
			else if(role == SomaticGeneRole::Role::AMBIGUOUS)
			{
				effect = "ambiguous";
			}
			else //do not report genes which have none of these effects
			{
				continue;
			}

			gene_effects <<  gene + ":" + effect;
		}
		stream << (gene_effects.empty() ? "NA" : gene_effects.join(";") );

		stream << endl;
	}
	somatic_cnvs_qbic->close();
}

void SomaticReportHelper::somaticSvForQbic(QString path_target_folder)
{
	QSharedPointer<QFile> somatic_sv_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_somatic_sv.tsv");

	QTextStream stream(somatic_sv_qbic.data());

	stream << "type" << "\t" << "gene" << "\t" << "effect" << "\t" << "left_bp" << "\t" << "right_bp" << endl;

	somatic_sv_qbic->close();

}

void SomaticReportHelper::metaDataForQbic(QString path_target_folder)
{
	QSharedPointer<QFile> meta_data_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_metadata.tsv");

	QTextStream stream(meta_data_qbic.data());

	stream << "diagnosis" << "\t" << "tumor_content" << "\t" << "pathogenic_germline" << "\t" << "mutational_load" << "\t";
	stream << "chromosomal_instability" << "\t" << "quality_flags" << "\t" << "reference_genome";
	stream << endl;

	stream << icd10_diagnosis_code_ << "\t" << histol_tumor_fraction_ << "\t";

	//No report of pathogenic germline variants
	stream << "NA" << "\t";

	if(mutation_burden_ < 3.3)
	{
		stream << "low";
	}
	else if(mutation_burden_ < 23.1 && mutation_burden_ >= 3.3)
	{
		stream << "medium";
	}
	else if(mutation_burden_ >= 23.1)
	{
		stream << "high";
	}
	stream << "\t";
	stream << "\t";
	stream << "\t";

	stream << db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(settings_.tumor_ps)).genome;
	stream << endl;

	meta_data_qbic->close();

}

VariantTranscript SomaticReportHelper::selectSomaticTranscript(const Variant& variant)
{
	QList<VariantTranscript> transcripts = variant.transcriptAnnotations(snv_index_coding_splicing_);

	//first coding/splicing transcript
	foreach(const VariantTranscript& trans, transcripts)
	{
		if(trans.typeMatchesTerms(obo_terms_coding_splicing_))
		{
			return trans;
		}
	}

	//first transcript
	if(transcripts.count()>0)
	{
		return transcripts[0];
	}

	return VariantTranscript();
}

QByteArray SomaticReportHelper::CnvTypeDescription(int tumor_cn)
{
	QByteArray type = "";

	if(tumor_cn > 2)
	{
		type = "AMP (" + QByteArray::number((int)tumor_cn) + " Kopien)";
	}
	else if(tumor_cn < 2)
	{
		type = "DEL";
		if(tumor_cn == 0) type.append(" (hom)");
		else if(tumor_cn == 1) type.append(" (het)");
	}
	else if(tumor_cn == 2) type = "LOH";
	else type = "NA";

	return type;
}

RtfSourceCode SomaticReportHelper::CnvDescription(const CopyNumberVariant& cnv, const SomaticGeneRole& role)
{
	int cn = cnv.copyNumber(cnvs_.annotationHeaders());

	RtfSourceCode out;

	if(role.role == SomaticGeneRole::Role::ACTIVATING && cn > 2)
	{
		if(role.high_evidence) out = "onkogene Veränderung";
		else out = "wahrscheinlich onkogene Veränderung";
	}
	else if(role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION && cn < 2)
	{
		if(role.high_evidence) out = "onkogene Veränderung";
		else out = "wahrscheinlich onkogene Veränderung";
	}
	else
	{
		out = "unklare Signifikanz der Veränderung";
	}

	if(cnv.copyNumber(cnvs_.annotationHeaders()) == 2)
	{
		out += " " + RtfText("/ Verlust des Wildtypallels").highlight(3).RtfCode();
	}

	if(cn > 2.) out += "\n\\line\nmögl. Überexpression";
	else if(cn < 2.) out+= "\n\\line\nmögl. reduzierte Expression";
	else out += "\n\\line\nunklare Bedeutung für Expression";


	return out;
}

QByteArray SomaticReportHelper::cytoband(const CopyNumberVariant &cnv)
{
	QByteArray out = "";
	if(cnv_index_cytoband_ > -1)
	{
		QByteArrayList parts = cnv.annotations()[cnv_index_cytoband_].trimmed().split(',');
		std::sort(parts.begin(),parts.end());
		if(parts.count() == 1 && !parts[0].isEmpty()) out = parts[0];
		else if(parts.count() > 1) out = parts.first() + parts.last(); //ISCN 2016 nomenclature
	}
	return out;
}

RtfTableRow SomaticReportHelper::overlappingCnv(const CopyNumberVariant &cnv, QByteArray gene, double snv_af)
{
	int cn = cnv.copyNumber(cnvs_.annotationHeaders());

	RtfTableRow row;
	row.addCell(1000, gene, RtfParagraph().setItalic(true));

	double tum_maximum_clonality = getCnvMaxTumorClonality(cnvs_);

	RtfText statement = RtfText("");
	if(cn > 2)
	{
		if(snv_af < tum_maximum_clonality/2.) statement.append("AMP WT (" + QByteArray::number(cn) + " Kopien)");
		else if(snv_af > tum_maximum_clonality/2.) statement.append("AMP MUT (" + QByteArray::number(cn) + " Kopien)");
	}
	else if(cn < 2)
	{
		if(cn == 1 ) statement.append("DEL (het)");
		else if(cn == 0) statement.append("DEL (hom)");
	}
	else
	{
		statement.append(cnv.annotations()[cnv_index_state_]);
	}
	statement.setFontSize(18);

	statement.append(RtfText(cnv.chr().strNormalized(true)).setFontSize(14).RtfCode(),true); //chromosome in new line
	if(cnv_index_cytoband_ > -1)
	{
		statement.append(RtfText("; " + cytoband(cnv)).setFontSize(14).RtfCode());
	}

	row.addCell(2400,statement.RtfCode());

	QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_);

	if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";

	row.addCell(1600,cnv_type);

	row.addCell(900,QByteArray::number(cnv.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2).replace(".", ","),RtfParagraph().setHorizontalAlignment("c"));
	row.addCell(4021, CnvDescription(cnv, db_.getSomaticGeneRole(gene)) );
	return row;
}

double SomaticReportHelper::getCnvMaxTumorClonality(const CnvList &cnvs)
{
	int i_cnv_tum_clonality = cnvs.annotationIndexByName("tumor_clonality", false);
	if(i_cnv_tum_clonality == -1 || cnvs.isEmpty()) return std::numeric_limits<double>::quiet_NaN();

	double tum_maximum_clonality = -1;
	for(int j=0;j<cnvs.count();++j)
	{
		if(cnvs[j].annotations().at(i_cnv_tum_clonality).toDouble() > tum_maximum_clonality)
		{
			tum_maximum_clonality = cnvs[j].annotations().at(i_cnv_tum_clonality).toDouble();
		}
	}

	return tum_maximum_clonality;
}

double SomaticReportHelper::getTumorContentBySNVs()
{
	double tumor_molecular_proportion;
	try
	{
		 tumor_molecular_proportion = tumor_qcml_data_.value("QC:2000054",true).toString().toDouble();
	}
	catch(...)
	{
		tumor_molecular_proportion = std::numeric_limits<double>::quiet_NaN();
	}

	if(tumor_molecular_proportion > 100.)	tumor_molecular_proportion = 100.;

	return tumor_molecular_proportion;
}

RtfTable SomaticReportHelper::pharamacogeneticsTable()
{
	RtfTable table;

	const static QMultiMap< QByteArray, QPair<QByteArray,QByteArray> > data = {
		{"rs1142345",QPair<QByteArray,QByteArray>("Toxizität","Cisplatin")},
		{"rs1142345",QPair<QByteArray,QByteArray>("Wirksamkeit","Cisplatin, Cyclophosphamide")},
		{"rs12248560",QPair<QByteArray,QByteArray>("Toxizität","Cyclophosphamid, Doxorubicin, Fluoruracil")},
		{"rs1800460",QPair<QByteArray,QByteArray>("Toxizität","Cisplatin")},
		{"rs3745274",QPair<QByteArray,QByteArray>("Dosierung","Cyclophosphamide, Doxorubicin")},
		{"rs3892097",QPair<QByteArray,QByteArray>("Wirksamkeit, Toxizität","Tamoxifen")},
		{"rs35742686", QPair<QByteArray, QByteArray>("Stoffwechsel", "Tamoxifen")},
		{"rs3918290",QPair<QByteArray,QByteArray>("Wirksamkeit","Fluoruracil")},
		{"rs3918290",QPair<QByteArray,QByteArray>("Toxizität, Stoffwechsel","Capecitabine, Fluoruracil, Pyrimidine analogues, Tegafur")},
		{"rs4148323",QPair<QByteArray,QByteArray>("Dosierung","Irinotecan")},
		{"rs4148323",QPair<QByteArray,QByteArray>("Sonstige","SN-38 (irinotecan metabolite)")},
		{"rs4148323",QPair<QByteArray,QByteArray>("Sonstige","Irinotecan")},
		{"rs4149056",QPair<QByteArray,QByteArray>("Toxizität","Irinotecan")},
		{"rs4149056",QPair<QByteArray,QByteArray>("Toxizität","Cyclophosphamid, Docetaxel, Doxorubicin, Epirubicin, Fluoruracil")},
		{"rs4244285",QPair<QByteArray,QByteArray>("Toxizität","Cyclophosphamid, Doxorubicin")},
		{"rs4244285",QPair<QByteArray,QByteArray>("Wirksamkeit","Cyclophosphamid, Doxorubicin")},
		{"rs4244285",QPair<QByteArray,QByteArray>("Stoffwechsel","Nelfinavir")},
		{"rs55886062",QPair<QByteArray,QByteArray>("Toxizität","Capecitabine, Fluoruracil, Pyrimidine analogues, Tegafur")},
		{"rs56038477",QPair<QByteArray,QByteArray>("Toxizität","Capecitabine, Fluoruracil")},
		{"rs67376798",QPair<QByteArray,QByteArray>("Toxizität, Stoffwechsel","Capecitabine, Fluoruracil, Pyrimidine analogues, Tegafur")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Toxizität","irinotecan")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Sonstige","SN-38 (irinotecan metaboite)")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Dosierung","Irinotecan")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Stoffwechsel","Belinostat")}
	};

	int i_dbsnp = germline_vl_.annotationIndexByName("dbSNP",true,false);
	int i_co_sp = germline_vl_.annotationIndexByName("coding_and_splicing",true,false);
	int i_genotype = germline_vl_.getSampleHeader().infoByStatus(true).column_index;

	if(i_dbsnp == -1)
	{
		return table;
	}


	for(int i=0;i<germline_vl_.count();++i)
	{
		const Variant& snv = germline_vl_[i];

		for(const auto& key : data.uniqueKeys())
		{
			if(snv.annotations().at(i_dbsnp).contains(key))
			{
				for(const auto& value : data.values(key))
				{
					RtfTableRow row;

					VariantTranscript trans = snv.transcriptAnnotations(i_co_sp)[0];

					row.addCell(1200,snv.annotations().at(i_dbsnp),RtfParagraph().setFontSize(14));
					row.addCell(800,trans.gene,RtfParagraph().setFontSize(14));

					if(!trans.hgvs_c.isEmpty() && !trans.hgvs_p.isEmpty())
					{
						row.addCell(1800,trans.hgvs_c + ", " + trans.hgvs_p,RtfParagraph().setFontSize(14));
					}
					else //use genomic position if no AA change available
					{
						row.addCell(1800, "g." + QByteArray::number(snv.start()) + snv.ref() + ">" + snv.obs(), RtfParagraph().setFontSize(14) );
					}

					row.addCell(800,snv.annotations().at(i_genotype),RtfParagraph().setFontSize(14));
					row.addCell(1300,value.first,RtfParagraph().setFontSize(14));
					row.addCell(4021,value.second,RtfParagraph().setFontSize(14));
					table.addRow(row);
				}
			}
		}
	}

	if(table.count() != 0) //Set style and header row in case we have found SNPs
	{
		table.prependRow(RtfTableRow({"RS-Nummer","Gen","Veränderung","Genotyp","Relevanz","Assoziierte Stoffe"},{1200,800,1800,800,1300,4021},RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());
		table.prependRow(RtfTableRow({"Pharmakogenetisch relevante Polymorphismen"},doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4).setHeader());
		table.setUniqueBorder(1,"brdrhair",4);
		table.addRow(RtfTableRow("Nähere Informationen erhalten Sie aus der Datenbank pharmGKB (https://www.pharmgkb.org)",{doc_.maxWidth()},RtfParagraph().setFontSize(14)));
	}

	return table;
}

RtfTable SomaticReportHelper::snvTable(const VariantList &vl, bool include_germline, bool include_cnvs)
{
	RtfTable table;

	//headlines
	QByteArray heading_text = "Punktmutationen (SNVs), kleine Insertionen/Deletionen (INDELs) und Kopienzahlvarianten (CNVs)";
	table.addRow(RtfTableRow(heading_text,doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4).setHeader());
	table.addRow(RtfTableRow({"Gen","Veränderung","Typ","Anteil","Beschreibung"},{1000,2400,1600,900,4021},RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader());

	GeneSet cna_already_included; //Copy number altered genes that are already included in RTF table

	//germline SNVs
	//Make list of somatic variants in control tissue, chosen according report settings
	VariantList som_var_in_normal = SomaticReportSettings::filterGermlineVariants(germline_vl_, settings_);
	if(include_germline)
	{
		//Add germline snvs to temporary set
		int i_germl_gene = som_var_in_normal.annotationIndexByName("gene");
		int i_germl_co_sp = som_var_in_normal.annotationIndexByName("coding_and_splicing");
		int i_germl_freq_in_tum = som_var_in_normal.annotationIndexByName("freq_in_tum");
		int i_germl_hom_het = som_var_in_normal.annotationIndexByName(settings_.normal_ps);


		for(int i=0; i< som_var_in_normal.count(); ++i) //insert next to gene that is already included
		{
			const Variant& var = som_var_in_normal[i];

			RtfTableRow row;
			row.addCell(1000, var.annotations()[i_germl_gene] + "\\super#", RtfParagraph().setItalic(true) );

			//Select germline transcript
			QList<VariantTranscript> transcripts =  var.transcriptAnnotations(i_germl_co_sp);
			VariantTranscript transcript;
			if(!transcripts.isEmpty())
			{
				transcript = transcripts[0];
				for(int j=0; j<transcripts.count(); ++j)
				{
					if(settings_.preferred_transcripts.value( transcripts[j].gene ).contains(transcripts[j].idWithoutVersion()) )
					{
						transcript = transcripts[j];
						break;
					}
				}
			}

			row.addCell({ transcript.hgvs_c + ", " + transcript.hgvs_p, RtfText(transcript.id).setFontSize(14).RtfCode()}, 2400);
			row.addCell(1600, transcript.type.replace("_variant",""));
			row.addCell(900, QByteArray::number(var.annotations()[i_germl_freq_in_tum].toDouble(), 'f', 2).replace(".",","), RtfParagraph().setHorizontalAlignment("c"));

			//Description of germline variant
			QByteArray germl_desc = "pathogene Variante (ACMG)";
			if(var.annotations()[i_germl_hom_het].contains("het")) germl_desc +=  ", in der Normalprobe heterozygot";
			else if(var.annotations()[i_germl_hom_het].contains("hom")) germl_desc +=  ", in der Normalprobe homozygot";
			else germl_desc += ", nachgewiesen im Normalgewebe";
			row.addCell( 4021, germl_desc );


			ebm_genes_ << transcript.gene;
			table.addRow(row);

			//Find overlapping somatic CNV for each SNV
			for(int i=0; i<cnvs_.count(); ++i)
			{
				const CopyNumberVariant& cnv = cnvs_[i];
				if(!cnv.overlapsWith(var.chr(), var.start(), var.end())) continue;
				//Skip copy altered genes that are already included in RTF table
				if(cna_already_included.contains(transcript.gene)) continue;
				cna_already_included << transcript.gene;
				//set first cell of corresponding cnv (contains gene name) as end of cell over multiple rows
				double tumor_af = var.annotations()[i_germl_hom_het].contains("het") ? 0.5 : 1.;
				table.addRow(overlappingCnv(cnv, transcript.gene, tumor_af));
			}
		}
	}


	//somatic SNVs
	int i_som_rep_alt = vl.annotationIndexByName("alt_var_alteration", true, false);
	int i_som_rep_desc = vl.annotationIndexByName("alt_var_description", true, false);
	int i_tum_af = vl.annotationIndexByName("tumor_af");
	int i_vicc = vl.annotationIndexByName("NGSD_som_vicc_interpretation");
	for(int i=0; i<vl.count();++i)
	{
		const Variant& snv = vl[i];

		VariantTranscript transcript = selectSomaticTranscript(snv);
		transcript.type = transcript.type.replace("_variant","");
		transcript.type.replace("&",", ");

		RtfTableRow row;
		row.addCell(1000,transcript.gene,RtfParagraph().setItalic(true));

		//In case there is som. report annotation, overwrite protein change in RTF table
		if(i_som_rep_alt > -1 && i_som_rep_desc > -1 && (snv.annotations().at(i_som_rep_alt) != "" || snv.annotations().at(i_som_rep_desc) != ""))
		{
			row.addCell(2400, snv.annotations()[i_som_rep_alt] + ", " + snv.annotations()[i_som_rep_desc]);
		}
		else //no annotation entry from somatic report conf
		{
			QList<RtfSourceCode> alterations;
			if(!transcript.hgvs_c.isEmpty()) alterations << transcript.hgvs_c;
			if(!transcript.hgvs_p.isEmpty()) alterations << transcript.hgvs_p;
			row.addCell({alterations.join(", "), RtfText(transcript.id).setFontSize(14).RtfCode()},2400);
		}

		row.last().format().setLineSpacing(276);

		row.addCell(1600,transcript.type);


		double tumor_af = snv.annotations().at(i_tum_af).toDouble();

		row.addCell(900,QByteArray::number(tumor_af, 'f', 2).replace(".",","), RtfParagraph().setHorizontalAlignment("c")); //tumor allele frequency

		QByteArray var_description = trans(snv.annotations()[i_vicc]);
		row.addCell(4021, (var_description.isEmpty() ? "nicht bewertet" : var_description) );

		ebm_genes_ << transcript.gene;
		table.addRow(row);


		//Find overlapping CNV for each SNV
		for(int i=0; i<cnvs_.count(); ++i)
		{
			const CopyNumberVariant& cnv = cnvs_[i];
			if(!cnv.overlapsWith(snv.chr(), snv.start(),snv.end())) continue;
			//Skip copy altered genes that are already included in RTF table
			if(cna_already_included.contains(transcript.gene)) continue;
			cna_already_included << transcript.gene;
			//set first cell of corresponding cnv (contains gene name) as end of cell over multiple rows
			table.addRow(overlappingCnv(cnv, transcript.gene, tumor_af));
		}
	}

	//Move overlapping CNVs to the end of variants of the same gene
	for(int i=2; i<table.count(); ++i)
	{
		if( !table[i][1].format().content().contains("AMP") && !table[i][1].format().content().contains("DEL") && !table[i][1].format().content().contains("LOH")) continue;
		if(i<table.count()-1)
		{
			if(table[i][0].format().content() == table[i+1][0].format().content()) //next row has the same gene symbol
			{
				table.swapRows(i, i+1);
			}
		}
	}

	//Merge cells with the same gene name
	for(int r=2; r<table.count(); ++r)
	{
		if(table[r-1][0].format().content() == table[r][0].format().content())
		{
			//set first row cell that contains gene name as cell over multiple rows as "clvmgf"
			if(table[r-1][0].controlWord().isEmpty()) table[r-1][0].setHeaderControlWord("clvmgf");

			//set second row cell as cell in center / at end of merge
			table[r][0].setHeaderControlWord("clvmrg");
		}
	}


	//Add CNVs
	if(include_cnvs)
	{
		QVector<RtfTableRow> cnv_rows;
		for(int i=0;i<cnvs_.count();++i)
		{
			const CopyNumberVariant& cnv = cnvs_[i];
			int cn = cnv.copyNumber(cnvs_.annotationHeaders());

			if(cn == 2) continue; //Skip LOHs

			GeneSet genes = db_.genesToApproved( cnv.genes() ).intersect(settings_.processing_system_genes);

			for(const auto& gene : genes)
			{
				if(db_.getSomaticGeneRoleId(gene) == -1) continue; //Do not include genes without defined gene role

				SomaticGeneRole gene_role = db_.getSomaticGeneRole(gene);

				if( !SomaticCnvInterpreter::includeInReport(cnvs_, cnv, gene_role) ) continue;

				if(!gene_role.high_evidence) continue; //Skip genes that are not high evidence

				if(cna_already_included.contains(gene)) continue; //Skip genes already included with overlapping SNV

				//Skip low cna genes, these will be printed in text hint.
				if(cn == 3)
				{
					skipped_amp_ << gene;
					continue;
				}

				RtfTableRow row;
				row.addCell(1000, gene, RtfParagraph().setItalic(true));

				RtfText cn_statement( CnvTypeDescription(cn) );

				cn_statement.append(RtfText(cnv.chr().strNormalized(true)).setFontSize(14).RtfCode(), true);

				if(cnv_index_cytoband_ > -1 ) cn_statement.append(RtfText("; " + cytoband(cnv)).setFontSize(14).RtfCode());


				row.addCell(2400, cn_statement.RtfCode());

				QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_);
				if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";
				row.addCell(1600, cnv_type);

				QByteArray tumor_clonality = QByteArray::number( cnv.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2 ).replace(".", ",");
				row.addCell(900, tumor_clonality, RtfParagraph().setHorizontalAlignment("c"));

				row.addCell(4021, CnvDescription(cnv, gene_role));

				ebm_genes_ << gene;
				cnv_rows << row;
			}
		}

		//sort CNV rows according gene name
		std::sort(cnv_rows.begin(), cnv_rows.end(), [](const RtfTableRow& rhs, const RtfTableRow& lhs){return rhs[0].format().content() < lhs[0].format().content();});
		for(const auto& row : cnv_rows)
		{
			table.addRow(row);
		}
	}

	table.setUniqueBorder(1,"brdrhair",4);



	//Add table description
	RtfSourceCode desc = "";
	desc += RtfText("Veränderung: ").setBold(true).setFontSize(14).RtfCode();
	desc += "Kodierende Position, " + RtfText("SNV").setBold(true).setFontSize(14).RtfCode() + " Punktmutationen " + RtfText("(single nucleotide variant), ").setItalic(true).setFontSize(14).RtfCode();
	desc += RtfText("INDELs").setFontSize(14).setBold(true).RtfCode() + " Insertionen/Deletionen, " + RtfText("CNV").setFontSize(14).setBold(true).RtfCode() + " Kopienzahlvariante, ";
	desc += RtfText("AMP").setFontSize(14).setBold(true).RtfCode() + " Amplifikation, " + RtfText("DEL").setFontSize(14).setBold(true).RtfCode() + " Deletion ";
	desc += RtfText("LOH").setFontSize(14).setBold(true).RtfCode() + " Kopienneutraler Verlust der Heterozygotie, " + RtfText("WT").setFontSize(14).setBold(true).RtfCode() + " Wildtypallel, ";
	desc += RtfText("MUT").setFontSize(14).setBold(true).RtfCode() + " Mutiertes Allel; ";
	desc += RtfText("Typ:").setFontSize(14).setBold(true).RtfCode() + " Art der SNV oder Größe und Ausdehnung der CNV: " + RtfText("focal").setFontSize(14).setBold(true).RtfCode() + " ( bis zu 3 Gene), ";
	desc += RtfText("Cluster").setFontSize(14).setBold(true).RtfCode() + " (weniger als 25% des Chromosomenarms) " + RtfText("non-focal").setFontSize(14).setBold(true).RtfCode() + " (Chromosomenanteil); ";
	desc += RtfText("Anteil:").setFontSize(14).setBold(true).RtfCode() + " Anteil der Allele mit der gelisteten Variante (SNV, INDEL) bzw. Anteil der Zellen mit der entsprechenden Kopienzahlvariante (CNV) in der ";
	desc += "untersuchten Probe; ";
	desc += RtfText("Beschreibung: ").setFontSize(14).setBold(true).RtfCode();
	desc += "Informationen aus Datenbanken (z.B. COSMIC, Cancerhotspots, Cancer Genome Interpreter, PubMed, OnkoKB, ClinVar, OMIM, VarSome, LOVD, HGMD) zu der Variante und funktionelle Daten werden integriert und die Onkogenität der Veränderung wird nach " + RtfText("Variant Interpretation for Cancer Consortium").setFontSize(14).setItalic(true).RtfCode() +" (VICC)-Richtlinien bewertet. In dieser Tabelle sind nur ";
	desc += (include_cnvs ? "onkogene" : "unklare");
	desc += " Veränderungen dargestellt.";
	if(som_var_in_normal.count() > 0 && include_germline) desc += "\n\\line\n{\\super#} auch in der Normalprobe nachgewiesen.";

	table.addRow(RtfTableRow(desc,{doc_.maxWidth()},RtfParagraph().setFontSize(14).setHorizontalAlignment("j")));
	return table;
}

void SomaticReportHelper::storeRtf(const QByteArray& out_file)
{
	/************
	 * METADATA *
	 ************/
	RtfTable general_info_table;
	general_info_table.addRow(RtfTableRow("Allgemeine genetische Charakteristika (" + RtfText(settings_.tumor_ps.toUtf8() + "-" + settings_.normal_ps.toUtf8()).setFontSize(16).setBold(false).RtfCode() +")",doc_.maxWidth(),RtfParagraph().setBold(true)).setBackgroundColor(4).setBorders(1,"brdrhair",4));

	QByteArray tumor_content_bioinf = "";
	if(settings_.report_config.tumContentByClonality()) tumor_content_bioinf = QByteArray::number(getCnvMaxTumorClonality(cnvs_) * 100., 'f', 0) + " \%";
	if(settings_.report_config.tumContentByMaxSNV())
	{
		double tumor_molecular_proportion = getTumorContentBySNVs();
		if(tumor_content_bioinf != "") tumor_content_bioinf += ", ";

		tumor_content_bioinf += QByteArray::number(tumor_molecular_proportion, 'f', 1) + " \%";
	}

	if(!settings_.report_config.tumContentByClonality() && !settings_.report_config.tumContentByMaxSNV())
	{
		tumor_content_bioinf = "nicht bestimmbar";
	}

	QByteArray tumor_content_hist = "nicht bestimmbar";
	if(settings_.report_config.tumContentByHistological())
	{
		tumor_content_hist = QByteArray::number(histol_tumor_fraction_, 'f', 0) + " \%";
	}
	general_info_table.addRow(RtfTableRow({"Tumoranteil (hist./bioinf.)", tumor_content_hist + " / " + tumor_content_bioinf}, {2500, 7421}).setBorders(1,"brdrhair", 4));

	RtfText mutation_burden_text(QByteArray::number(mutation_burden_).replace(".", ",") + " Var/Mbp");
	if(settings_.report_config.tmbReferenceText() != "")
	{
		mutation_burden_text.append(";");
		mutation_burden_text.append(RtfText("Vergleichswerte: " + settings_.report_config.tmbReferenceText().toUtf8()).setFontSize(14).RtfCode(), true);
	}

	general_info_table.addRow(RtfTableRow({"Mutationslast", mutation_burden_text.RtfCode()}, {2500,7421}).setBorders(1,"brdrhair",4));
	general_info_table.last()[0].setBorder(1,1,1,0,"brdrhair");
	general_info_table.last().last().setBorder(1,1,1,0,"brdrhair");


	//MSI status, values larger than 0.16 are considered unstable
	if(settings_.report_config.msiStatus())
	{
		general_info_table.addRow(RtfTableRow({"Mikrosatelliten", ( mantis_msi_swd_value_ <= 0.16 ? "kein Hinweis auf eine MSI" : "Hinweise auf MSI" ) },{2500,7421}).setBorders(1,"brdrhair",4));
	}

	//Fusion status
	if(settings_.report_config.fusionsDetected())
	{
		general_info_table.addRow(RtfTableRow({"Fusionen/Strukturvarianten", RtfText("Hinweise auf eine Strukturvariante in XXX").highlight(3).RtfCode()}, {2500,7421}).setBorders(1, "brdrhair", 4));
	}
	else
	{
		general_info_table.addRow(RtfTableRow({"Fusionen/Strukturvarianten", "nicht nachgewiesen"}, {2500,7421}).setBorders(1, "brdrhair", 4));
	}

	//Virus DNA status
	QByteArrayList virus_names;
	for(const auto& virus : validated_viruses_)
	{
		virus_names << virus.virusName();
	}
	general_info_table.addRow(RtfTableRow({"Virus-DNA", (virus_names.count() > 0 ? "Hinweise auf " + virus_names.join(", ") : "nicht nachgewiesen")}, {2500,7421}).setBorders(1, "brdrhair", 4));


	//Calc percentage of CNV altered genome
	if(settings_.report_config.cnvBurden())
	{
		RtfSourceCode text_cnv_burden = "";
		double cnv_altered_percentage = cnvBurden(cnvs_);
		if(cnv_altered_percentage >= 0.01)
		{
			text_cnv_burden = QByteArray::number(cnv_altered_percentage,'f',0) + " \%" ;
		}
		else
		{
			text_cnv_burden = "CNVs aufgrund des niedrigen Tumorgehaltes nicht/eingeschränkt bestimmbar";
		}

		if(settings_.report_config.cinChromosomes().count() > 0)
		{
			QList<QString> chr = settings_.report_config.cinChromosomes();

			//Sort chromosomes naturally
			QCollator coll;
			coll.setNumericMode(true);
			std::sort(chr.begin(), chr.end(), [&](const QString s1, const QString& s2){return coll.compare(s1,s2) < 0;});

			RtfSourceCode temp = "\\line Verdacht auf einechromosomale Instabilität: Chr. ";
			for(int i=0; i< settings_.report_config.cinChromosomes().count(); ++i)
			{
				if( i< settings_.report_config.cinChromosomes().count() - 2) temp += chr[i].toUtf8().replace("chr","") + ", ";
				else if(i == settings_.report_config.cinChromosomes().count() -2 ) temp += chr[i].toUtf8().replace("chr","") + " und ";
				else temp += chr[i].toUtf8().replace("chr","");
			}
			temp +=".";
			text_cnv_burden += RtfText(temp).setFontSize(14).RtfCode();
		}
		else
		{
			text_cnv_burden += RtfText("\\line Es gibt keine Hinweise auf eine chromosomale Instabilität.").setFontSize(14).RtfCode();
		}

		general_info_table.addRow(RtfTableRow({"CNV-Last", text_cnv_burden},{2500,7421},RtfParagraph()).setBorders(1,"brdrhair",4));
	}


	general_info_table.addRow(RtfTableRow({"HRD-Score", QByteArray::number(settings_.report_config.hrdScore()) + RtfText("\\line Ein Wert \\u8805;3 weist auf eine HRD hin.").setFontSize(14).RtfCode()}, {2500,7421},  RtfParagraph()).setBorders(1, "brdrhair", 4));

	if(settings_.report_config.quality() != "no abnormalities")
	{
		general_info_table.addRow(RtfTableRow({"Anmerkungen", trans(settings_.report_config.quality()).toUtf8()}, {2500, 7421}, RtfParagraph()).setBorders(1, "brdrhair", 4));
	}

	RtfSourceCode desc = "";
	desc += RtfText("Tumoranteil").setFontSize(14).setBold(true).RtfCode() + " (hist.): Von der Pathologie mitgeteilt; (bioinf.): Die bioinformatische Berechnung des Tumoranteils beruht auf dem Anteil der nachgewiesenen SNVs oder CNVs. ";
	desc += RtfText("Mutationslast:").setFontSize(14).setBold(true).RtfCode() + " Anzahl der Varianten in den kodierenden untersuchten Genen normiert auf eine Million Basenpaare; ";
	desc += RtfText("Mikrosatelliten:").setFontSize(14).setBold(true).RtfCode() + " Bewertung der Mikrosatelliteninstabilität; ";
	desc += RtfText("CNV-Last:").setFontSize(14).setBold(true).RtfCode() + " Anteil des Genoms, bei dem die Kopienzahl verändert ist. ";
	desc += RtfText("HRD-Score:").setFontSize(14).setBold(true).RtfCode() + " Homologer Rekombinations-Defizienz-Score.";
	general_info_table.addRow(RtfTableRow(desc, doc_.maxWidth(), RtfParagraph().setFontSize(14).setHorizontalAlignment("j")).setBorders(0) );


	doc_.addPart(general_info_table.RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());


	/********************************
	 * RELEVANT SOMATIC ALTERATIONS *
	 ********************************/
	int i_som_vicc = somatic_vl_.annotationIndexByName("NGSD_som_vicc_interpretation");
	VariantList vl_high_significance;
	vl_high_significance.copyMetaData(somatic_vl_);


	//fill variant list of high significance
	GeneSet high_significance_genes; //all genes that are highly significant e.g. because they have a SNV of high significance

	//high significance table
	for(int i=0; i<somatic_vl_.count(); ++i)
	{
		const Variant& snv = somatic_vl_[i];
		QByteArray vicc = snv.annotations()[i_som_vicc];
		if(vicc == "ONCOGENIC" || vicc == "LIKELY_ONCOGENIC")
		{
			vl_high_significance.append(snv);
			high_significance_genes << selectSomaticTranscript(snv).gene;
		}

	}
	//high significance: SNVs of uncertain significance that have another SNV of high confidence in the same gene
	for(int i=0; i<somatic_vl_.count(); ++i)
	{
		const Variant& snv = somatic_vl_[i];
		QByteArray gene = selectSomaticTranscript(snv).gene;
		if(high_significance_genes.contains(gene) && !vl_high_significance.contains(snv))
		{
			vl_high_significance.append(snv);
		}
	}
	//high significance: SNVs that lie in a gene that is to be included in
	for(int i=0; i<cnvs_.count(); ++i)
	{
		const CopyNumberVariant& cnv = cnvs_[i];
		GeneSet genes = cnv.genes();
		for(const auto& gene : genes)
		{
			if( db_.getSomaticGeneRoleId(gene) == -1) continue;
			SomaticGeneRole  role = db_.getSomaticGeneRole(gene);

			if(SomaticCnvInterpreter::includeInReport(cnvs_, cnv, role) && role.high_evidence) //CNA gene is of high significance
			{
				for(int j=0; j<somatic_vl_.count(); ++j)
				{
					const Variant& snv = somatic_vl_[j];

					if(gene == selectSomaticTranscript(snv).gene && !vl_high_significance.contains(snv))
					{
						vl_high_significance.append(snv);
						break;
					}
				}
			}
		}
	}

	vl_high_significance.sortByAnnotation(vl_high_significance.annotationIndexByName("gene"));
	doc_.addPart(RtfParagraph("Potentiell relevante somatische Veränderungen:").setBold(true).setIndent(0,0,0).setSpaceBefore(250).RtfCode());
	RtfTable high_significant_table = snvTable(vl_high_significance, true, true);
	doc_.addPart(high_significant_table.RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());



	//SNVs of low significance
	VariantList vl_low_significance;
	vl_low_significance.copyMetaData(somatic_vl_);
	for(int i=0; i<somatic_vl_.count(); ++i)
	{
		const Variant& var = somatic_vl_[i];
		if(!vl_high_significance.contains(var))
		{
			vl_low_significance.append(var);
		}
	}
	vl_low_significance.sortByAnnotation(vl_low_significance.annotationIndexByName("gene"));


	//Write hint in case of unclassified variants
	bool unclassified_snvs = false;
	for(int i=0; i<vl_high_significance.count();++i)
	{
		if(vl_high_significance[i].annotations()[i_som_vicc].trimmed().isEmpty())
		{
			unclassified_snvs = true;
			break;
		}
	}

	for(int i=0; i<vl_low_significance.count(); ++i)
	{
		if(vl_low_significance[i].annotations()[i_som_vicc].trimmed().isEmpty())
		{
			unclassified_snvs = true;
			break;
		}
	}


	if(unclassified_snvs)
	{
		doc_.addPart(RtfParagraph("Es wurden sehr viele somatische Varianten nachgewiesen, die zu einer hohen Mutationslast führen. Da die Wechselwirkungen aller Varianten nicht eingeschätzt werden können, wird von der funktionellen Bewertung einzelner Varianten abgesehen. Falls erforderlich kann die Bewertung nachgereicht werden.").highlight(3).RtfCode());
		doc_.addPart(RtfParagraph("").RtfCode());
	}

	if(skipped_amp_.count() > 0)
	{
		RtfSourceCode text = "Weiterhin wurden nicht-fokale Amplifikationen (3 Kopien) für die folgenden relevanten Gene nachgewiesen: ";
		std::sort(skipped_amp_.begin(), skipped_amp_.end());
		text += RtfText(skipped_amp_.join(", ")).setFontSize(18).setItalic(true).RtfCode();
		text += ".";
		doc_.addPart(RtfParagraph(text).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());
		doc_.addPart(RtfParagraph("").setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setFontSize(18).RtfCode());
	}



	if(settings_.report_config.fusionsDetected())
	{
		RtfSourceCode snv_expl = "Es gibt Hinweise auf eine Deletion/Fusion/Translokation/Strukturvariante, die zu einer Fusion/Deletion/... führen könnte.";
		doc_.addPart(RtfParagraph(snv_expl).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).setBold(true).highlight(3).RtfCode());
		doc_.addPart(RtfParagraph("").setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setFontSize(18).RtfCode());
	}
	RtfSourceCode snv_expl = "";

	//support for limitation text
	snv_expl = RtfText("Limitationen: ").setBold(true).setFontSize(18).RtfCode();
	if(settings_.report_config.limitations().isEmpty())
	{
		snv_expl += "Die Probenqualität zeigt keine Auffälligkeiten. Methodisch bedingte Limitationen sind im Anhang erläutert.";
	}
	else
	{
		snv_expl += settings_.report_config.limitations().replace("\n","\n\\line\n");
	}
	doc_.addPart(RtfParagraph(snv_expl).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setHorizontalAlignment("j").RtfCode());


	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setFontSize(18).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).RtfCode());

	snv_expl = RtfText("Zusätzliche genetische Daten:").setFontSize(18).setBold(true).RtfCode();
	snv_expl += " Weitere Informationen zu allen nachgewiesenen somatischen Veränderungen und pharmakogenetisch relevanten Polymorphismen entnehmen Sie bitte der Anlage. ";
	doc_.addPart(RtfParagraph(snv_expl).highlight(3).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());
	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setLineSpacing(276).setFontSize(18).RtfCode());

	snv_expl = "Die Varianten- und Gendosisanalysen der Gene " + RtfText("BRCA1").setItalic(true).setFontSize(18).RtfCode() + " und " + RtfText("BRCA2").setItalic(true).setFontSize(18).RtfCode();
	snv_expl += " in der Normalprobe waren unauffällig.";
	doc_.addPart(RtfParagraph(snv_expl).highlight(3).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());
	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setLineSpacing(276).setFontSize(18).RtfCode());

	doc_.addPart(RtfParagraph("Die Analyse des Transkriptoms wird getrennt berichtet.").highlight(3).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());
	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setLineSpacing(276).setFontSize(18).RtfCode());

	doc_.addPart(RtfParagraph("Über die somatische Analyse einer zusätzlichen Tumorprobe wird nachträglich berichtet.").highlight(3).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());

	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setLineSpacing(276).setFontSize(18).RtfCode());

	doc_.newPage();

	/*********************
	 * ADDITIONAL REPORT *
	 *********************/

	doc_.addPart( RtfParagraph("Varianten unklarer Onkogenität:").setBold(true).setIndent(0,0,0).setSpaceBefore(250).RtfCode() );
	RtfTable low_significant_table = snvTable(vl_low_significance, false, false);
	doc_.addPart(low_significant_table.RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());


	//Make rtf report, filter genes for processing system target region
	doc_.addPart(cnvTable().RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());

	/***********
	 * FUSIONS *
	 ***********/
	if(settings_.report_config.fusionsDetected())
	{
		RtfTable fusion_table;
		fusion_table.addRow(RtfTableRow("Strukturvarianten",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader().setBackgroundColor(4));

		fusion_table.addRow(RtfTableRow({"Variante", "Genomische Bruchpunkte", "Beschreibung"}, {1700, 3000, 5221}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());
		fusion_table.addRow(RtfTableRow({"", "", "", ""}, {1700,1500,1500,5221}, RtfParagraph().setFontSize(16)));
		fusion_table.setUniqueBorder(1,"brdrhair",4);

		doc_.addPart(fusion_table.RtfCode());
	}

	doc_.addPart(RtfParagraph("").RtfCode());
	/***************
	 * VIRUS TABLE *
	 ***************/
	if(validated_viruses_.count() > 0)
	{
		RtfTable virus_table;
		virus_table.addRow(RtfTableRow("Virale DNA",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4));
		virus_table.addRow(RtfTableRow({"Virus","Gen","Genom","Region","Abdeckung","Bewertung"},{963,964,1927,1927,1927,1929},RtfParagraph().setBold(true)));
		for(const auto& virus : validated_viruses_)
		{
			RtfTableRow row;

			row.addCell(963,virus.virusName());

			row.addCell(964,virus.virusGene());
			row.addCell(1927,virus.chr);

			QByteArray region = QByteArray::number(virus.start) + "-" + QByteArray::number(virus.end);
			row.addCell(1927,region);
			row.addCell(1927,QByteArray::number(virus.coverage,'f',1));
			row.addCell(1929,"nachgewiesen*");

			virus_table.addRow(row);
		}

		virus_table.setUniqueBorder(1,"brdrhair",4);
		virus_table.addRow(RtfTableRow("*Wir empfehlen eine Bestätigung des nachgewiesenen Onkovirus mit einer validierten Methode, beispielsweise am Institut für Medizinische Virologie und Epidemiologie der Viruskrankheiten Tübingen.",doc_.maxWidth(),RtfParagraph().setSpaceBefore(50).setFontSize(14)));
		virus_table.last().setBorders(0);
		doc_.addPart(virus_table.RtfCode());
		doc_.addPart(RtfParagraph("").RtfCode());
	}

	/**************************
	 * PHARMACOGENOMICS TABLE *
	 **************************/
	doc_.addPart(pharamacogeneticsTable().RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());

	/*******************************************
	 * GENERAL INFORMATION / QUALITY PARAMETERS*
	 *******************************************/
	RtfTable metadata;
	metadata.addRow(RtfTableRow({RtfText("Allgemeine Informationen").setBold(true).setFontSize(16).RtfCode(),RtfText("Qualitätsparameter").setBold(true).setFontSize(16).RtfCode()}, {4550,5371}));
	metadata.addRow(RtfTableRow({"Auswertungsdatum:",QDate::currentDate().toString("dd.MM.yyyy").toUtf8(),"Analysepipeline:", somatic_vl_.getPipeline().toUtf8()}, {1550,3000,2250,3121}));
	metadata.addRow(RtfTableRow({"Proben-ID (Tumor):", settings_.tumor_ps.toUtf8(), "Auswertungssoftware:", QCoreApplication::applicationName().toUtf8() + " " + QCoreApplication::applicationVersion().toUtf8()},{1550,3000,2250,3121}));
	metadata.addRow(RtfTableRow({"Proben-ID (Keimbahn):", settings_.normal_ps.toUtf8(), "Coverage Tumor 100x:", tumor_qcml_data_.value("QC:2000030",true).toString().toUtf8() + " \%"},{1550,3000,2250,3121}));
	metadata.addRow(RtfTableRow({"MSI-Status:", (!BasicStatistics::isValidFloat(mantis_msi_swd_value_) ? "n/a" : QByteArray::number(mantis_msi_swd_value_,'f',3)),  "Durchschnittliche Tiefe Tumor:", tumor_qcml_data_.value("QC:2000025",true).toString().toUtf8() + "x"},{1550,3000,2250,3121}));


	metadata.addRow(RtfTableRow({"Prozessierungssystem:",processing_system_data_.name.toUtf8() + " (" + QByteArray::number(settings_.processing_system_genes.count()) + ")", "Coverage Normal 100x:", normal_qcml_data_.value("QC:2000030",true).toString().toUtf8() + " \%"} , {1550,3000,2250,3121} ));
	metadata.addRow(RtfTableRow({"ICD10:", icd10_diagnosis_code_.toUtf8(), "Durchschnittliche Tiefe Normal:", normal_qcml_data_.value("QC:2000025",true).toString().toUtf8() + "x"},{1550,3000,2250,3121}));

	metadata.addRow(RtfTableRow("In Regionen mit einer Abdeckung >100x können somatische Varianten mit einer Frequenz >5% im Tumorgewebe mit einer Sensitivität >95,0% und einem Positive Prediction Value PPW >99% bestimmt werden. Für mindestens 95% aller untersuchten Gene kann die Kopienzahl korrekt unter diesen Bedingungen bestimmt werden.", doc_.maxWidth()) );

	metadata.setUniqueFontSize(14);

	doc_.addPart(metadata.RtfCode());



	/***********************
	 * BILLING INFORMATION *
	 ***********************/
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.newPage();
	doc_.addPart(RtfParagraph("Abrechnungsinformation gemäß einheitlicher Bewertungsmaßstab").setBold(true).RtfCode());
	doc_.addPart(billingTable().RtfCode());


	doc_.save(out_file);
}

void SomaticReportHelper::storeXML(QString file_name)
{
	VariantList som_var_in_normal = SomaticReportSettings::filterGermlineVariants(germline_vl_, settings_);
	SomaticXmlReportGeneratorData data(settings_, somatic_vl_, som_var_in_normal, cnvs_);

	data.processing_system_roi = settings_.processing_system_roi;
	data.processing_system_genes = settings_.processing_system_genes;

	data.tumor_content_histology = histol_tumor_fraction_ / 100.; //is stored as double between 0 and 1, NGSD contains percentages
	data.tumor_content_snvs = getTumorContentBySNVs() / 100; //is stored as a double between 0 and 1, QCML file contains percentages
	data.tumor_content_clonality = getCnvMaxTumorClonality(cnvs_) ;
	data.tumor_mutation_burden = mutation_burden_;
	data.mantis_msi = mantis_msi_swd_value_;


	QByteArray xml_data = SomaticXmlReportGenerator::generateXML(data, db_, false).toUtf8();
	QSharedPointer<QFile> out_file = Helper::openFileForWriting(file_name);
	out_file->write(xml_data);
	out_file->close();
}

void SomaticReportHelper::storeQbicData(QString path)
{
	if(!QDir(path).exists())
	{
		QDir().mkdir(path);
	}

	germlineSnvForQbic(path);
	somaticSnvForQbic(path);
	germlineCnvForQbic(path);
	somaticCnvForQbic(path);
	somaticSvForQbic(path);
	metaDataForQbic(path);
}


QString SomaticReportHelper::trans(const QString &text)
{
	QHash<QString, QString> en2de;
	en2de["no abnormalities"] = "keine Auffälligkeiten";
	en2de["tumor cell content too low"] = "Tumorzellgehalt zu niedrig";
	en2de["quality of tumor DNA too low"] = "Qualität der Tumor-DNA zu gering";
	en2de["DNA quantity too low"] = "DNA-Menge im Tumor zu gering";
	en2de["heterogeneous sample"] = "Probe heterogen";
	en2de["activating"] = "aktivierend";
	en2de["test_dependent"] = "testabhängige Bedeutung";
	en2de["ONCOGENIC"] = "onkogene Variante";
	en2de["LIKELY_ONCOGENIC"] = "wahrscheinlich onkogene Variante";
	en2de["BENIGN"] = "gutartige Variante";
	en2de["LIKELY_BENIGN"] = "wahrscheinlich gutartige Variante";
	en2de["UNCERTAIN_SIGNIFICANCE"] = "unklare Variante";
	en2de["loss_of_function"] = "Funktionsverlust";
	en2de["ambiguous"] = "unklare Bedeutung";


	if(!en2de.contains(text)) return text; //return original entry if not found

	return en2de[text];
}
