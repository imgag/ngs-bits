#include "SomaticReportHelper.h"
#include "BasicStatistics.h"
#include "OntologyTermCollection.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "SomaticReportSettings.h"
#include "NGSD.h"
#include "XmlHelper.h"
#include "SomaticXmlReportGenerator.h"
#include "SomaticVariantInterpreter.h"
#include "LoginManager.h"
#include "ApiCaller.h"
#include "ClientHelper.h"
#include <cmath>
#include <QDir>
#include <QMap>
#include <QPair>
#include <QCollator>
#include <QJsonDocument>
#include <QJsonArray>


SomaticReportHelper::SomaticReportHelper(GenomeBuild build, const VariantList& variants, const CnvList &cnvs, const VariantList& variants_germline, const SomaticReportSettings& settings, bool test_db)
	: build_(build)
	, settings_(settings)
	, germline_vl_(variants_germline)
	, cnvs_()
	, validated_viruses_()
	, db_(test_db)
{
	//Assign SNV annotation indices
	snv_index_coding_splicing_ = variants.annotationIndexByName("coding_and_splicing");
	somatic_vl_ = SomaticReportSettings::filterVariants(variants, settings); //filtered out snvs flagged as artefacts
	somatic_vl_.sortByAnnotation(somatic_vl_.annotationIndexByName("gene"));

	//high significance genes: VICC classification
	GeneSet important_genes;
	int i_som_vicc = somatic_vl_.annotationIndexByName("NGSD_som_vicc_interpretation");
	for(int i=0; i<somatic_vl_.count(); ++i)
	{
		const Variant& variant = somatic_vl_[i];
		QByteArray vicc = variant.annotations()[i_som_vicc];
		if(vicc == "ONCOGENIC" || vicc == "LIKELY_ONCOGENIC")
		{
			important_genes << selectSomaticTranscript(variant, settings_, snv_index_coding_splicing_).gene;
		}
	}

	//Filter CNVs according report configuration settings
	cnvs_ = SomaticReportSettings::filterCnvs(cnvs, settings);

	//high significance genes: with reported CNV
	for(int i=0; i<cnvs_.count(); ++i)
	{
		const CopyNumberVariant& cnv = cnvs_[i];
		for(const QByteArray& gene : cnv.genes())
		{
			//only genes with high evidence role
			SomaticGeneRole role = db_.getSomaticGeneRole(gene);
			if (!role.isValid() || !role.high_evidence) continue;

			//only if included in report
			if(!SomaticCnvInterpreter::includeInReport(cnvs_, cnv, role)) continue;

			important_genes << gene;
		}
	}

	//create lists of important and not imporant variants
	for(int i=0; i<somatic_vl_.count(); ++i)
	{
		QByteArray gene = selectSomaticTranscript(somatic_vl_[i], settings_, snv_index_coding_splicing_).gene;
		if(important_genes.contains(gene))
		{
			somatic_vl_high_impact_indices_ << i;
		}
		else
		{
			somatic_vl_low_impact_indices_ << i;
		}
	}
	low_impact_indices_converted_to_high_.clear();

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

	//Load virus data if available
	try
	{
		if (!settings_.viral_file.isEmpty())
		{
			TSVFileStream file(settings_.viral_file);
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

				if(tmp.coverage < 50) continue;

				validated_viruses_ << tmp;

			}
		}

	}
	catch(...) {} //Nothing to do here

	//assign CNV annotation indices
	cnv_index_cn_change_ = cnvs_.annotationIndexByName("CN_change", false);
	cnv_index_cnv_type_ = cnvs_.annotationIndexByName("cnv_type", false);
	cnv_index_tumor_clonality_ = cnvs_.annotationIndexByName("tumor_clonality", false);
	cnv_index_state_ = cnvs_.annotationIndexByName("state", false);
	cnv_index_cytoband_ = cnvs_.annotationIndexByName("cytoband", false);

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
		if(entry.type == "tumor fraction") tmp.append(entry.disease_info);
	}
	if(tmp.count() == 1) histol_tumor_fraction_ = tmp[0].toDouble();
	else histol_tumor_fraction_ = std::numeric_limits<double>::quiet_NaN();
	tmp.clear();


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
	addColors(doc_);
}


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


RtfSourceCode SomaticReportHelper::partCnvTable()
{
	RtfSourceCode output;

	RtfTable cnv_table;

	//Table Header
	cnv_table.addRow(RtfTableRow({"Chromosomale Aberrationen"},doc_.maxWidth(),RtfParagraph().setHorizontalAlignment("c").setBold(true).setFontSize(18)).setBackgroundColor(4).setHeader());
	cnv_table.addRow(RtfTableRow({"Position","CNV","Typ","CN","Anteil","Gene"},{1800,900,900,400,800,5121},RtfParagraph().setHorizontalAlignment("c").setFontSize(16).setBold(true)).setHeader());

	RtfParagraph header_format;
	header_format.setBold(true);
	header_format.setHorizontalAlignment("c");

	if(cnvs_.isEmpty())
	{
		cnv_table.removeRow(1);
		QString limits = settings_.report_config.limitations();
		if (limits.contains("Tumorgehalt niedrig") || limits.contains("niedrigem Anteil an Tumorzellen") || limits.contains("geringen Tumorgehaltes"))
		{
			cnv_table.addRow(RtfTableRow("CNV waren aufgrund des niedrigen Tumorgehaltes nicht bestimmbar.",doc_.maxWidth()));
		}
		else
		{
			cnv_table.addRow(RtfTableRow("Es wurden keine CNVs gefunden.",doc_.maxWidth()));
		}

		cnv_table.setUniqueBorder(1,"brdrhair",4);
		return cnv_table.RtfCode();
	}

	if(cnv_index_tumor_clonality_ < 0)
	{
		cnv_table.addRow(RtfTableRow("Die ClinCNV-Datei enthält keine Tumor Clonality. Bitte mit einer aktuelleren Version von ClinCNV neu berechnen.",doc_.maxWidth()));
		cnv_table.setUniqueBorder(1,"brdrhair",4);
		return cnv_table.RtfCode();
	}


	for(int i=0; i<cnvs_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_[i];

		if(settings_.target_region_filter.isValid() && !settings_.target_region_filter.regions.overlapsWith( variant.chr(),variant.start(), variant.end() ) ) continue; //target region from GSvar filter widget
		if(variant.genes().count() == 0) continue;

		//gene names
		GeneSet genes = settings_.target_region_filter.genes.intersect( db_.genesOverlapping(variant.chr(), variant.start(), variant.end()) );
		std::sort(genes.begin(), genes.end());
		if(genes.count() == 0) continue;

		RtfTableRow temp_row;

		//coordinates
		QList<RtfSourceCode> coords;
		coords << RtfText(variant.chr().str()).setFontSize(14).RtfCode();
		coords << RtfText(QByteArray::number(variant.start() == 0 ? 1 : variant.start()) + " - " + QByteArray::number(variant.end())).setFontSize(12).RtfCode();
		temp_row.addCell(1800, coords);

		//AMP/DEL
		QByteArray var_length = (variant.size() / 1000000. < 0.1) ? "<0.1 MB" : QByteArray::number( variant.size() / 1000000. , 'f', 1) + " MB";
		QList<RtfSourceCode> cnv_desc;
		cnv_desc << RtfText(CnvTypeDescription(variant.copyNumber(cnvs_.annotationHeaders()), false)).setFontSize(14).RtfCode();
		cnv_desc << RtfText("(" + var_length + ")").setFontSize(12).RtfCode();
		temp_row.addCell(900, cnv_desc, RtfParagraph().setHorizontalAlignment("c"));

		//Type
		RtfSourceCode type_statement = variant.annotations().at(cnv_index_cnv_type_);
		type_statement = type_statement.replace("chromosome", "chr");
		type_statement = type_statement.replace("partial p-arm", "partial p-arm");
		type_statement = type_statement.replace("partial q-arm", "partial q-arm");

		type_statement += "\n\\line" +RtfText(cytoband(variant)).setFontSize(12).RtfCode();
		temp_row.addCell(900, type_statement, RtfParagraph().setHorizontalAlignment("c").setFontSize(14));

		//copy numbers
		temp_row.addCell(400,QByteArray::number(variant.copyNumber(cnvs_.annotationHeaders())), RtfParagraph().setFontSize(14).setHorizontalAlignment("c"));

		//tumor clonality
		temp_row.addCell(800,QByteArray::number(variant.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2).replace(".", ","), RtfParagraph().setHorizontalAlignment("c").setFontSize(14));

		temp_row.addCell(5121,genes.join(", "), RtfParagraph().setItalic(true).setFontSize(14));

		cnv_table.addRow(temp_row);
	}
	cnv_table.setUniqueBorder(1,"brdrhair",4);

	output.append(cnv_table.RtfCode());

	//legend
	RtfSourceCode desc = RtfText("CNV:").setBold(true).setFontSize(14).RtfCode() + RtfText(" Kopienzahlvariante, ").setFontSize(14).RtfCode();
	desc += RtfText("AMP:").setBold(true).setFontSize(14).RtfCode() + RtfText(" Amplifikation, ").setFontSize(14).RtfCode();
	desc += RtfText("DEL:").setBold(true).setFontSize(14).RtfCode() + RtfText(" Deletion, ").setFontSize(14).RtfCode();
	desc += RtfText("LOH:").setBold(true).setFontSize(14).RtfCode() + RtfText(" Kopienzahlneutraler Verlust der Heterozygotie, ").setFontSize(14).RtfCode();
	desc += RtfText("CN:").setBold(true).setFontSize(14).RtfCode() + RtfText(" Copy Number, ").setFontSize(14).RtfCode();
	desc += RtfText("Anteil:").setBold(true).setFontSize(14).RtfCode() + RtfText(" Anteil der Zellen mit der entsprechenden Kopienzahlvariante in der untersuchten Probe.").setFontSize(14).RtfCode();

	output += RtfParagraph(desc).setHorizontalAlignment("j").RtfCode();

	return output;
}

RtfSourceCode SomaticReportHelper::partBillingTable()
{
	RtfTable table;

	QByteArray heading_text = "Abrechnungsinformation gemäß einheitlicher Bewertungsmaßstab";
	table.addRow(RtfTableRow(heading_text,doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4).setHeader());

	table.addRow(RtfTableRow({"Gen", "OMIM"}, {doc_.maxWidth()/2, doc_.maxWidth()/2}, RtfParagraph().setHorizontalAlignment("c").setFontSize(16).setBold(true)).setHeader() );

	BedFile target = settings_.target_region_filter.regions;
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

	foreach(const auto& gene, ebm_genes_)
	{
		QByteArrayList omim_mims;
		foreach(const auto& info,  db_.omimInfo(gene) )
		{
			omim_mims << info.mim;
		}
		table.addRow( RtfTableRow({gene, omim_mims.join(", ")},{doc_.maxWidth()/2, doc_.maxWidth()/2}) );
	}
	table.setUniqueBorder(1,"brdrhair",4);

	table.addRow( RtfTableRow("Basenpaare der abzurechnenden Gene: " + QByteArray::number(size), doc_.maxWidth(), RtfParagraph().setFontSize(14)).setBorders(0) );

	return table.RtfCode();
}


void SomaticReportHelper::germlineSnvForQbic(QString path_target_folder)
{
	//currently no germline SNVs are uploaded, only created header
	QByteArray content;
	QTextStream stream(&content);

	stream << "chr" << "\t" << "start" << "\t" << "ref" << "\t" << "alt" << "\t" << "genotype" << "\t";
	stream << "gene" << "\t" << "base_change" << "\t" << "aa_change" << "\t" << "transcript" << "\t";
	stream << "functional_class" << "\t" << "effect";
	stream << endl;

	saveReportData("QBIC_germline_snv.tsv", path_target_folder, content);
}

void SomaticReportHelper::somaticSnvForQbic(QString path_target_folder)
{
	FastaFileIndex genome_reference(Settings::string("reference_genome", false));
	QByteArray content;
	QTextStream stream(&content);

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

		VcfLine vcf_rep = variant.toVCF(genome_reference);
		stream << vcf_rep.chr().str() << "\t";
		stream << vcf_rep.start() << "\t";
		stream << vcf_rep.ref() << "\t";
		stream << vcf_rep.altString() << "\t";
		stream << variant.annotations().at(i_tumor_af) << "\t";
		stream << variant.annotations().at(i_tumor_depth) << "\t";

		//determine transcript, usually first coding/splicing
		VariantTranscript transcript = selectSomaticTranscript(variant, settings_, snv_index_coding_splicing_);

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
		if(db_.getSomaticViccId(variant)!= -1)
		{
			SomaticGeneRole gene_role = db_.getSomaticGeneRole(transcript.gene);
			if (gene_role.isValid())
			{
				SomaticVariantInterpreter::Result result = SomaticVariantInterpreter::viccScore(db_.getSomaticViccData(variant));
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

		}
		else effect = "NA";

		stream << effect;

		stream << endl;
	}
	saveReportData("QBIC_somatic_snv.tsv", path_target_folder, content);
}

void SomaticReportHelper::germlineCnvForQbic(QString path_target_folder)
{
	QByteArray content;
	QTextStream stream(&content);

	stream << "size" << "\t" << "type" << "\t" << "copy_number" << "\t" << "gene" << "\t" << "exons" << "\t" << "transcript" << "\t";
	stream << "chr" << "\t" << "start" << "\t" << "end" << "\t" << "effect";
	stream << endl;

	saveReportData("QBIC_germline_cnv.tsv", path_target_folder, content);
}


void SomaticReportHelper::somaticCnvForQbic(QString path_target_folder)
{
	QByteArray content;
	QTextStream stream(&content);

	stream << "size" << "\t" << "type" << "\t" << "copy_number" << "\t" << "gene" << "\t" << "exons" << "\t";
	stream << "transcript" << "\t" << "chr" << "\t" << "start" << "\t" << "end" << "\t" << "effect" << endl;

	for(int i=0; i < cnvs_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_[i];

		GeneSet genes = settings_.target_region_filter.genes.intersect(db_.genesOverlapping(variant.chr(), variant.start(), variant.end()));

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

		if(genes.count()>0)
		{
			stream << genes.join(";");
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

		QByteArrayList gene_effects;
		foreach(const auto& gene, genes)
		{
			SomaticGeneRole gene_role = db_.getSomaticGeneRole(gene);
			if (!gene_role.isValid()) continue;

			QByteArray effect = "";
			int cn = variant.copyNumber(cnvs_.annotationHeaders());

			if(cn > 2 && gene_role.role == SomaticGeneRole::Role::ACTIVATING)
			{
				effect = "activating";
			}
			else if(cn < 2 && gene_role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION)
			{
				effect = "inactivating";
			}
			else if(gene_role.role == SomaticGeneRole::Role::AMBIGUOUS)
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
	saveReportData("QBIC_somatic_cnv.tsv", path_target_folder, content);
}

void SomaticReportHelper::somaticSvForQbic(QString path_target_folder)
{
	QByteArray content;
	QTextStream stream(&content);

	stream << "type" << "\t" << "gene" << "\t" << "effect" << "\t" << "left_bp" << "\t" << "right_bp" << endl;

	saveReportData("QBIC_somatic_sv.tsv", path_target_folder, content);
}

void SomaticReportHelper::metaDataForQbic(QString path_target_folder)
{
	QByteArray content;
	QTextStream stream(&content);

	stream << "diagnosis" << "\t" << "tumor_content" << "\t" << "pathogenic_germline" << "\t" << "mutational_load" << "\t" << "chromosomal_instability" << "\t" << "quality_flags" << "\t" << "reference_genome";
	stream << endl;

	stream << settings_.icd10 << "\t" << (BasicStatistics::isValidFloat(histol_tumor_fraction_) ? QString::number(histol_tumor_fraction_, 'f', 4) : "NA") << "\t";

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

	saveReportData("QBIC_metadata.tsv", path_target_folder, content);
}

VariantTranscript SomaticReportHelper::selectSomaticTranscript(const Variant& variant, const SomaticReportSettings& settings, int index_co_sp)
{
	QList<VariantTranscript> transcripts = variant.transcriptAnnotations(index_co_sp);

	//use preferred transcript that is coding or splicing if available
	foreach(const VariantTranscript& trans, transcripts)
	{
		if(settings.preferred_transcripts.value( trans.gene ).contains(trans.idWithoutVersion()) && trans.typeMatchesTerms(settings.obo_terms_coding_splicing))
		{
			return trans;
		}
	}

	//first coding/splicing transcript otherwise
	foreach(const VariantTranscript& trans, transcripts)
	{
		if(trans.typeMatchesTerms(settings.obo_terms_coding_splicing))
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

void SomaticReportHelper::addColors(RtfDocument& doc)
{
	doc.addColor(188,230,138);
	doc.addColor(255,0,0);
	doc.addColor(255,255,0);
	doc.addColor(191,191,191);
	doc.addColor(240,240,240);
}

QByteArray SomaticReportHelper::CnvTypeDescription(int tumor_cn, bool add_cn)
{
	QByteArray type = "";

	if(tumor_cn > 2)
	{
		type = "AMP";
		if (add_cn) type += " (" + QByteArray::number((int)tumor_cn) + " Kopien)";
	}
	else if(tumor_cn < 2)
	{
		type = "DEL";
		if(add_cn && tumor_cn == 0)
		{
			type += " (hom)";
		}
		else if(add_cn && tumor_cn == 1)
		{
			type += " (het)";
		}
	}
	else if(tumor_cn == 2) type = "LOH";
	else type = "n/a";

	return type;
}

RtfSourceCode SomaticReportHelper::CnvDescription(const CopyNumberVariant& cnv, const SomaticGeneRole& role, double snv_tumor_af)
{
	int cn = cnv.copyNumber(cnvs_.annotationHeaders());

	RtfSourceCode out;

	if(role.role == SomaticGeneRole::Role::ACTIVATING && cn > 2)
	{
		if(role.high_evidence) out = "onkogene Veränderung";
		else out = "wahrsch. onkogene Veränderung";
	}
	else if(role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION && cn < 2)
	{
		if(role.high_evidence) out = "onkogene Veränderung";
		else out = "wahrsch. onkogene Veränderung";
	}
	else
	{
		out = "unklare Signifikanz der Veränderung";
	}

	if(cnv.copyNumber(cnvs_.annotationHeaders()) == 2)
	{
		if (snv_tumor_af == -1)
		{
			out += " " + RtfText("/ Verlust des Wildtypallels").highlight(3).RtfCode();
		}

		QByteArray cnv_type = CnvTypeDescription(cnv.copyNumber(cnvs_.annotationHeaders()), false);
		if ((cnv_type == "LOH" && cnv.annotations().at(cnv_index_tumor_clonality_).toDouble() >= snv_tumor_af*0.85)
			|| (cnv_type == "DEL" && cnv.annotations().at(cnv_index_tumor_clonality_).toDouble() >= getTumorContentBioinf()*0.85))
		{
			out = RtfText("Verlust des Wildtypallels").highlight(3).RtfCode();
		}
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

RtfTableRow SomaticReportHelper::overlappingCnv(const CopyNumberVariant &cnv, QByteArray gene, const QList<int>& col_widths, double snv_tumor_af)
{
	int cn = cnv.copyNumber(cnvs_.annotationHeaders());

	RtfTableRow row;
	row.addCell(col_widths[0], gene, RtfParagraph().setItalic(true));

	RtfText statement = RtfText("");
	if(cn > 2)
	{
		statement.append("AMP (" + QByteArray::number(cn) + " Kopien)");
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

	row.addCell(col_widths[1], statement.RtfCode());

	QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_);

	if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";

	row.addCell(col_widths[2], cnv_type);

	row.addCell(col_widths[3], QByteArray::number(cnv.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2).replace(".", ","),RtfParagraph().setHorizontalAlignment("c"));
	row.addCell(col_widths[4], CnvDescription(cnv, db_.getSomaticGeneRole(gene), snv_tumor_af));
	row.addCell(col_widths[5], db_.getSomaticPathwayGenes(gene).join(", "));

	return row;
}

void SomaticReportHelper::saveReportData(QString filename, QString path, QString content)
{
	if (!ClientHelper::isClientServerMode())
	{
		if(!QDir(path).exists()) QDir().mkdir(path);

		QSharedPointer<QFile> meta_data_qbic = Helper::openFileForWriting(path + "/" + filename);
		meta_data_qbic.data()->write(content.toUtf8());
		meta_data_qbic->close();
		return;
	}

	HttpHeaders qbic_headers;
	qbic_headers.insert("Accept", "application/json");
	qbic_headers.insert("Content-Type", "application/json");
	qbic_headers.insert("Content-Length", QByteArray::number(content.size()));

	// Exception is handled from GSvar
	RequestUrlParams params;
	params.insert("filename", QUrl(filename).toEncoded());
	params.insert("id", QUrl(path).toEncoded());
	ApiCaller().post("qbic_report_data", params, qbic_headers, content.toUtf8(), true, false, true);
}

double SomaticReportHelper::getCnvMaxTumorClonality(const CnvList &cnvs)
{
	int i_cnv_tum_clonality = cnvs.annotationIndexByName("tumor_clonality", false);
	if(i_cnv_tum_clonality == -1) return std::numeric_limits<double>::quiet_NaN();

	double tum_maximum_clonality = -1;
	for(int j=0; j<cnvs.count(); ++j)
	{
		bool ok = false;
		double tmp = cnvs[j].annotations().at(i_cnv_tum_clonality).toDouble(&ok);
		if(ok && tmp > tum_maximum_clonality)
		{
			tum_maximum_clonality = tmp;
		}
	}

	if (tum_maximum_clonality==-1) return std::numeric_limits<double>::quiet_NaN();

	return tum_maximum_clonality;
}

double SomaticReportHelper::getTumorContentBySNVs()
{
	try
	{
		double tumor_molecular_proportion = Helper::toDouble(tumor_qcml_data_.value("QC:2000054",true).toString(), "QC:2000054");
		return BasicStatistics::bound(tumor_molecular_proportion, 0.0, 100.0);
	}
	catch(...)
	{
		return std::numeric_limits<double>::quiet_NaN();
	}
}

RtfSourceCode SomaticReportHelper::partMetaData()
{
	RtfSourceCode out = RtfParagraph("Allgemeine Informationen").setBold(true).RtfCode();

	RtfTable metadata;
	metadata.addRow(RtfTableRow( {"", RtfText("Tumor").setFontSize(14).setUnderline(true).RtfCode(), RtfText("Normal").setFontSize(14).setUnderline(true).RtfCode(), "Prozessierungssystem:", processing_system_data_.name.toUtf8()}, {2000,1480,1480,1480,3481}, RtfParagraph().setFontSize(14)) );

	if(settings_.target_region_filter.name == "")
	{
		metadata.addRow(RtfTableRow({"Proben-ID", settings_.tumor_ps.toUtf8(), settings_.normal_ps.toUtf8(), "", ""}, {2000,1480,1480,1480,3481} , RtfParagraph().setFontSize(14)) );
	}
	else
	{
		QString panel_size = QString::number(settings_.target_region_filter.regions.baseCount()/1000000.0, 'f', 2);
		metadata.addRow(RtfTableRow({"Proben-ID", settings_.tumor_ps.toUtf8(), settings_.normal_ps.toUtf8(), "Genpanel:", settings_.target_region_filter.name.toUtf8() + "\n\\line\n(" + panel_size.toUtf8() + " MB, Gennamen s. letzte Seite)"}, {2000,1480,1480,1480,3481} , RtfParagraph().setFontSize(14)) );
	}

	metadata.addRow(RtfTableRow({"Durchschnittliche Tiefe:", tumor_qcml_data_.value("QC:2000025",true).toString().toUtf8() + "x", normal_qcml_data_.value("QC:2000025",true).toString().toUtf8() + "x", "Auswertungsdatum:", settings_.report_config.evaluationDate().toString("dd.MM.yyyy").toUtf8()}, {2000,1480,1480,1480,3481}) );


	RtfSourceCode tum_panel_depth = "n/a";
	RtfSourceCode nor_panel_depth = "n/a";

	//try to add somatic custom target depth stat
	try
	{
		tum_panel_depth = tumor_qcml_data_.value("QC:2000097",true).toString().toUtf8() + "x";
		nor_panel_depth = normal_qcml_data_.value("QC:2000097",true).toString().toUtf8() + "x";
	}
	catch(Exception) //nothing to do here
	{
	}

	metadata.addRow(RtfTableRow({"Durchschnittliche Tiefe Genpanel:", tum_panel_depth, nor_panel_depth, "Analysepipeline:", somatic_vl_.getPipeline().toUtf8()}, {2000,1480,1480,1480,3481}) );



	RtfSourceCode tum_cov_60x = "n/a";
	try
	{
		tum_cov_60x = tumor_qcml_data_.value("QC:2000099",true).toString().toUtf8() + "\%";
	}
	catch(Exception)
	{} //nothing to do here
	metadata.addRow( RtfTableRow( {"Coverage 60x:", tum_cov_60x, "", "Auswertungssoftware:", QCoreApplication::applicationName().toUtf8() + " " + QCoreApplication::applicationVersion().toUtf8()} , {2000,1480,1480,1480,3481}) );


	RtfSourceCode tum_panel_cov_60x = "n/a";


	//try to add somatic custom target 60x coverage stat
	try
	{
		tum_panel_cov_60x = tumor_qcml_data_.value("QC:2000098",true).toString().toUtf8() + "\%";
	}
	catch(Exception) //nothing to do here
	{
	}

	metadata.addRow(RtfTableRow({"Coverage Genpanel 60x:", tum_panel_cov_60x , "", "ICD10:", settings_.icd10.toUtf8()}, {2000,1480,1480,1480,3481}) );


	RtfSourceCode nor_panel_cov_20x = "n/a";
	try
	{
		nor_panel_cov_20x = normal_qcml_data_.value("QC:2000091",true).toString().toUtf8() + "\%";
	}
	catch(Exception) //nothing to do here
	{}

	RtfSourceCode nor_cov_20x = "n/a";
	try
	{
		nor_cov_20x = normal_qcml_data_.value("QC:2000027",true).toString().toUtf8() + "\%";
	}
	catch(Exception)
	{} //nothing to do here

	metadata.addRow(RtfTableRow({"Coverage 20x:", "" , nor_cov_20x, "MSI-Status:", (!BasicStatistics::isValidFloat(mantis_msi_swd_value_) ? "n/a" : QByteArray::number(mantis_msi_swd_value_,'f',3))}, {2000,1480,1480,1480,3481}));
	metadata.addRow(RtfTableRow({"Coverage Genpanel 20x:", "" , nor_panel_cov_20x, "Tumor-Ploidie:", (settings_.report_config.ploidy() == 0 ? "n/a" : QByteArray::number(settings_.report_config.ploidy(),'f',3))}, {2000,1480,1480,1480,3481}));

	metadata.addRow(RtfTableRow("In Regionen mit einer Abdeckung >60 können somatische Varianten mit einer Frequenz >5% im Tumorgewebe mit einer Sensitivität >95,0% und einem Positive Prediction Value PPW >99% bestimmt werden. Für mindestens 95% aller untersuchten Gene kann die Kopienzahl korrekt unter diesen Bedingungen bestimmt werden.", doc_.maxWidth()) );

	metadata.setUniqueFontSize(14);
	out.append(metadata.RtfCode());

	return out;
}

RtfSourceCode SomaticReportHelper::partVirusTable()
{
	RtfTable virus_table;
	virus_table.addRow(RtfTableRow("Virale DNA",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4));
	virus_table.addRow(RtfTableRow({"Virus","Gen","Genom","Region","Abdeckung","Bewertung"},{1000,1000,2000,1921,2000,2000},RtfParagraph().setBold(true)));
	foreach(const auto& virus, validated_viruses_)
	{
		RtfTableRow row;

		if (virus.idendity >= 90)
		{
			row.addCell(1000,virus.virusName());
		}
		else
		{
			row.addCell(1000,RtfText(virus.virusName()).highlight(3).RtfCode());
		}

		row.addCell(1000,virus.virusGene());
		row.addCell(2000,virus.chr);

		QByteArray region = QByteArray::number(virus.start) + "-" + QByteArray::number(virus.end);
		row.addCell(1921,region);
		row.addCell(2000,QByteArray::number(virus.coverage,'f',1));
		row.addCell(2000,"nachgewiesen*");

		virus_table.addRow(row);
	}

	virus_table.setUniqueBorder(1,"brdrhair",4);
	virus_table.addRow(RtfTableRow("*Wir empfehlen eine Bestätigung des nachgewiesenen Onkovirus mit einer validierten Methode, beispielsweise am Institut für Medizinische Virologie und Epidemiologie der Viruskrankheiten Tübingen.",doc_.maxWidth(),RtfParagraph().setSpaceBefore(50).setFontSize(14)));
	virus_table.last().setBorders(0);
	return virus_table.RtfCode();
}

RtfSourceCode SomaticReportHelper::partIgvScreenshot()
{
	if(settings_.igv_snapshot_png_hex_image == "") return "";

	RtfPicture snapshot(settings_.igv_snapshot_png_hex_image, settings_.igv_snapshot_width, settings_.igv_snapshot_height);

	snapshot.resizeToWidth(doc_.maxWidth());

	return snapshot.RtfCode();
}

RtfSourceCode SomaticReportHelper::partPharmacoGenetics()
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

	for(int i=0;i<germline_vl_.count();++i)
	{
		const Variant& snv = germline_vl_[i];

		foreach(const auto& key, data.uniqueKeys())
		{
			if(snv.annotations().at(i_dbsnp).contains(key))
			{
				foreach(const auto& value, data.values(key))
				{
					RtfTableRow row;

					VariantTranscript trans = snv.transcriptAnnotations(i_co_sp).at(0);

					if (key == "rs3918290" || key == "rs55886062" || key == "rs67376798" || key == "rs56038477")
					{
						row.addCell(1200,snv.annotations().at(i_dbsnp),RtfParagraph().setFontSize(14).highlight(3));
					}
					else
					{
						row.addCell(1200,snv.annotations().at(i_dbsnp),RtfParagraph().setFontSize(14));
					}


					if(!trans.gene.isEmpty())
					{
						row.addCell( 800, trans.gene, RtfParagraph().setFontSize(14).setItalic(true) );
					}
					else if(key == "rs12248560") //has no gene annotated in VEP, however, we need CYP2C19 here
					{
						row.addCell( 800, "CYP2C19", RtfParagraph().setFontSize(14).setItalic(true) );
					}
					else
					{
						row.addCell( 800, "n/a", RtfParagraph().setFontSize(14).setItalic(true) );
					}


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
		table.addRow(RtfTableRow("Nähere Informationen erhalten Sie aus der Datenbank pharmGKB (https://www.pharmgkb.org)",doc_.maxWidth(), RtfParagraph().setFontSize(14)));
	}
	else
	{
		table.addRow(RtfTableRow("Nicht nachgewiesen", doc_.maxWidth()));
		table.prependRow(RtfTableRow({"Pharmakogenetisch relevante Polymorphismen"},doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4).setHeader());
		table.setUniqueBorder(1,"brdrhair",4);
		return table.RtfCode();
	}

	return table.RtfCode();
}

RtfTable SomaticReportHelper::snvTable(const QSet<int>& indices, bool high_impact_table)
{
	QByteArrayList headers = {"Gen", "Veränderung", "Typ", "Anteil", "Beschreibung", "Molekularer Signalweg"};
	QList<int> col_widths = {1000, 1950, 1400, 600, 2950, 2022};

	//headlines
	RtfTable table;
	QByteArray heading_text = "Punktmutationen (SNVs), kleine Insertionen/Deletionen (INDELs) und Kopienzahlvarianten (CNVs)";
	table.addRow(RtfTableRow(heading_text,doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4).setHeader());
	table.addRow(RtfTableRow(headers, col_widths, RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader());

	GeneSet cna_already_included; //Copy number altered genes that are already included in RTF table

	//germline SNVs
	//Make list of somatic variants in control tissue, chosen according report settings
	VariantList som_var_in_normal = SomaticReportSettings::filterGermlineVariants(germline_vl_, settings_);

	if(high_impact_table)
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
			QByteArray gene = var.annotations()[i_germl_gene];
			row.addCell(col_widths[0], gene + "\\super#", RtfParagraph().setItalic(true) );

			//Select germline transcript
			QList<VariantTranscript> transcripts = var.transcriptAnnotations(i_germl_co_sp);
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

			row.addCell(col_widths[1], QByteArrayList() << transcript.hgvs_c + ", " + transcript.hgvs_p << RtfText(transcript.id).setFontSize(14).RtfCode());
			row.addCell(col_widths[2], transcript.type.replace("_variant",""));
			row.addCell(col_widths[3], QByteArray::number(var.annotations()[i_germl_freq_in_tum].toDouble(), 'f', 2).replace(".",","), RtfParagraph().setHorizontalAlignment("c"));

			//Description of germline variant
			QByteArray germl_desc = "pathogene Variante";
			if(var.annotations()[i_germl_hom_het].contains("het")) germl_desc +=  ", in der Normalprobe heterozygot";
			else if(var.annotations()[i_germl_hom_het].contains("hom")) germl_desc +=  ", in der Normalprobe homozygot";
			else germl_desc += ", nachgewiesen im Normalgewebe";
			row.addCell(col_widths[4], germl_desc );
			row.addCell(col_widths[5], db_.getSomaticPathways(gene).join(", "));
			table.addRow(row);

			ebm_genes_ << transcript.gene;

			//Find overlapping somatic CNV for each SNV
			for(int i=0; i<cnvs_.count(); ++i)
			{
				const CopyNumberVariant& cnv = cnvs_[i];
				if(!cnv.overlapsWith(var.chr(), var.start(), var.end())) continue;

				//Skip copy altered genes that are already included in RTF table
				if(cna_already_included.contains(transcript.gene)) continue;
				cna_already_included << transcript.gene;
				cnv_high_impact_indices_[i].insert(transcript.gene);

				//set first cell of corresponding cnv (contains gene name) as end of cell over multiple rows
				table.addRow(overlappingCnv(cnv, transcript.gene, col_widths, var.annotations()[i_germl_freq_in_tum].toDouble()));
			}

			foreach(int l, somatic_vl_low_impact_indices_)
			{
				const Variant& low_impact_snv = somatic_vl_[l];

				VariantTranscript transcript = selectSomaticTranscript(low_impact_snv, settings_, snv_index_coding_splicing_);
				transcript.type = transcript.type.replace("_variant","");
				transcript.type.replace("&",", ");

				if (! low_impact_indices_converted_to_high_.contains(l) && gene == transcript.gene)
				{
					table.addRow(snvRow(low_impact_snv, transcript, col_widths));
					low_impact_indices_converted_to_high_.insert(l);
				}
			}
		}
	}

	//somatic SNVs
	int i_tum_af = somatic_vl_.annotationIndexByName("tumor_af");
	QList<int> indices_sorted = indices.toList();
	std::sort(indices_sorted.begin(), indices_sorted.end());
	foreach(int i, indices_sorted)
	{
		if (! high_impact_table && low_impact_indices_converted_to_high_.contains(i)) continue;

		const Variant& snv = somatic_vl_[i];

		VariantTranscript transcript = selectSomaticTranscript(snv, settings_, snv_index_coding_splicing_);
		transcript.type = transcript.type.replace("_variant","");
		transcript.type.replace("&",", ");

		table.addRow(snvRow(snv, transcript, col_widths));


		ebm_genes_ << transcript.gene;


		//Find overlapping CNV for each SNV
		for(int i=0; i<cnvs_.count(); ++i)
		{
			const CopyNumberVariant& cnv = cnvs_[i];
			if(!cnv.overlapsWith(snv.chr(), snv.start(),snv.end())) continue;

			//Skip copy altered genes that are already included in RTF table
			if(cna_already_included.contains(transcript.gene)) continue;
			cna_already_included << transcript.gene;
			if (high_impact_table) cnv_high_impact_indices_[i].insert(transcript.gene);

			double tumor_af = snv.annotations().at(i_tum_af).toDouble();
			//set first cell of corresponding cnv (contains gene name) as end of cell over multiple rows
			table.addRow(overlappingCnv(cnv, transcript.gene, col_widths, tumor_af));
		}

		if (high_impact_table)
		{
			foreach(int l, somatic_vl_low_impact_indices_)
			{
				const Variant& low_impact_snv = somatic_vl_[l];

				VariantTranscript transcript_low_impact = selectSomaticTranscript(low_impact_snv, settings_, snv_index_coding_splicing_);
				transcript_low_impact.type = transcript.type.replace("_variant","");
				transcript_low_impact.type.replace("&",", ");

				if (! low_impact_indices_converted_to_high_.contains(l) && transcript.gene == transcript_low_impact.gene)
				{
					table.addRow(snvRow(low_impact_snv, transcript, col_widths));
					low_impact_indices_converted_to_high_.insert(l);
				}
			}
		}
	}

	//Move overlapping CNVs to the end of variants of the same gene
	for(int i=2; i<table.count()-1; ++i)
	{
		if( !table[i][1].format().content().contains("AMP") && !table[i][1].format().content().contains("DEL") && !table[i][1].format().content().contains("LOH")) continue;

		if(table[i][0].format().content() == table[i+1][0].format().content()) //next row has the same gene symbol
		{
			table.swapRows(i, i+1);
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

			//also merge pathway cells
			if(table[r-1][5].controlWord().isEmpty()) table[r-1][5].setHeaderControlWord("clvmgf");
			table[r][5].setHeaderControlWord("clvmrg");
		}
	}

	//Add CNVs
	if(high_impact_table)
	{
		QVector<RtfTableRow> cnv_rows;
		for(int i=0;i<cnvs_.count();++i)
		{
			const CopyNumberVariant& cnv = cnvs_[i];
			int cn = cnv.copyNumber(cnvs_.annotationHeaders());

			if(cn == 2) continue; //Skip LOHs

			if( settings_.target_region_filter.isValid() && !settings_.target_region_filter.regions.overlapsWith( cnv.chr(), cnv.start(), cnv.end() ) ) continue; //target region from GSvar filter widget

			GeneSet genes = settings_.target_region_filter.genes.intersect(db_.genesOverlapping(cnv.chr(), cnv.start(), cnv.end()));

			foreach(const auto& gene, genes)
			{
				//skip genes without defined gene role
				SomaticGeneRole gene_role = db_.getSomaticGeneRole(gene);
				if (!gene_role.isValid()) continue;

				if( !SomaticCnvInterpreter::includeInReport(cnvs_, cnv, gene_role) ) continue;

				if(!gene_role.high_evidence) continue; //Skip genes that are not high evidence

				if(cna_already_included.contains(gene)) continue; //Skip genes already included with overlapping SNV

				//Skip low cna genes, these will be printed in text hint.
				if(cn == 3)
				{
					skipped_amp_ << gene;
					continue;
				}

				//add row
				RtfTableRow row;
				row.addCell(col_widths[0], gene, RtfParagraph().setItalic(true));

				RtfText cn_statement( CnvTypeDescription(cn, true) );

				cn_statement.append(RtfText(cnv.chr().strNormalized(true)).setFontSize(14).RtfCode(), true);

				if(cnv_index_cytoband_ > -1 ) cn_statement.append(RtfText("; " + cytoband(cnv)).setFontSize(14).RtfCode());


				row.addCell(col_widths[1], cn_statement.RtfCode());

				QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_);
				if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";
				row.addCell(col_widths[2], cnv_type);

				QByteArray tumor_clonality = QByteArray::number( cnv.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2 ).replace(".", ",");
				row.addCell(col_widths[3], tumor_clonality, RtfParagraph().setHorizontalAlignment("c"));

				row.addCell(col_widths[4], CnvDescription(cnv, gene_role));

				row.addCell(col_widths[5], db_.getSomaticPathways(gene).join(", "));

				cnv_rows << row;

				//update datastructures for next parts
				ebm_genes_ << gene;
				cnv_high_impact_indices_[i].insert(gene);
			}
		}

		//sort CNV rows according gene name
		std::sort(cnv_rows.begin(), cnv_rows.end(), [](const RtfTableRow& rhs, const RtfTableRow& lhs){return rhs[0].format().content() < lhs[0].format().content();});
		foreach(const auto& row, cnv_rows)
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
	desc += (high_impact_table ? "onkogene" : "unklare");
	desc += " Veränderungen dargestellt.";
	if(som_var_in_normal.count() > 0 && high_impact_table) desc += "\n\\line\n{\\super#} auch in der Normalprobe nachgewiesen.";

	table.addRow(RtfTableRow(desc,doc_.maxWidth(),RtfParagraph().setFontSize(14).setHorizontalAlignment("j")));

	return table;
}

RtfTableRow SomaticReportHelper::snvRow(const Variant& snv, const VariantTranscript& transcript, const QList<int>& col_widths)
{
	int i_som_rep_alt = somatic_vl_.annotationIndexByName("alt_var_alteration", true, false);
	int i_som_rep_desc = somatic_vl_.annotationIndexByName("alt_var_description", true, false);
	int i_tum_af = somatic_vl_.annotationIndexByName("tumor_af");
	int i_vicc = somatic_vl_.annotationIndexByName("NGSD_som_vicc_interpretation");

	RtfTableRow row;
	QByteArray gene = transcript.gene;
	row.addCell(col_widths[0],transcript.gene,RtfParagraph().setItalic(true));

	//In case there is som. report annotation, overwrite protein change in RTF table
	if(i_som_rep_alt > -1 && i_som_rep_desc > -1 && (snv.annotations().at(i_som_rep_alt) != "" || snv.annotations().at(i_som_rep_desc) != ""))
	{
		row.addCell(col_widths[1], snv.annotations()[i_som_rep_alt] + ", " + snv.annotations()[i_som_rep_desc]);
	}
	else //no annotation entry from somatic report conf
	{
		QList<RtfSourceCode> alterations;
		if(!transcript.hgvs_c.isEmpty()) alterations << transcript.hgvs_c;
		if(!transcript.hgvs_p.isEmpty()) alterations << transcript.hgvs_p;
		if (alterations.isEmpty()) alterations << RtfText("???").highlight(3).RtfCode();
		row.addCell(col_widths[1], QByteArrayList() << alterations.join(", ") << RtfText(transcript.id).setFontSize(14).RtfCode());
	}

	row.last().format().setLineSpacing(276);

	row.addCell(col_widths[2], prepareTranscriptType(transcript.type));

	row.addCell(col_widths[3],QByteArray::number(snv.annotations().at(i_tum_af).toDouble(), 'f', 2).replace(".",","), RtfParagraph().setHorizontalAlignment("c")); //tumor allele frequency

	QByteArray var_description = trans(snv.annotations()[i_vicc]);
	row.addCell(col_widths[4], (var_description.isEmpty() ? "nicht bewertet" : var_description) );

	row.addCell(col_widths[5], db_.getSomaticPathways(gene).join(", "));

	return row;
}

QString SomaticReportHelper::getHlaFilepath(QString ps_name)
{
	QString hla_file;
	if (!ClientHelper::isClientServerMode())
	{
		hla_file = db_.processedSamplePath(db_.processedSampleId(ps_name), PathType::HLA_GENOTYPER);
	}
	else
	{
		HttpHeaders ps_headers;
		ps_headers.insert("Accept", "application/json");
		ps_headers.insert("Content-Type", "application/json");
		RequestUrlParams params;
		params.insert("ps_id", db_.processedSampleId(ps_name).toUtf8());
		params.insert("type", FileLocation::typeToString(PathType::HLA_GENOTYPER).toUtf8());

		QByteArray reply = ApiCaller().get("processed_sample_path", params, ps_headers, true, false, true);
		QJsonDocument json_doc = QJsonDocument::fromJson(reply);
		QJsonArray json_array = json_doc.array();
		for (int i = 0; i < json_array.count(); i++)
		{
			if (!json_array.at(i).isObject()) break;
			if (json_array.at(i).toObject().contains("filename"))
			{
				hla_file = json_array.at(i).toObject().value("filename").toString();
			}
		}
	}

	if (hla_file.isEmpty())
	{
		THROW(DatabaseException, "hla file for the processed sample '" + ps_name + "' was not found!");
	}

	return hla_file;
}


RtfTable SomaticReportHelper::hlaTable(QString ps_tumor, QString ps_normal)
{
	SomaticHlaInfo tumor_hla = SomaticHlaInfo(getHlaFilepath(ps_tumor));
	SomaticHlaInfo normal_hla = SomaticHlaInfo(getHlaFilepath(ps_normal));

	RtfTable table;
	table.addTitelRow({"HLA"},{doc_.maxWidth()});
	table.addHeaderRow({"Gene","Blut (" + ps_normal.toUtf8() + ")", "Tumor (" + ps_tumor.toUtf8() + ")"}, {1522, 4200, 4200});

	foreach(QByteArray gene, QByteArrayList({"HLA-A", "HLA-B", "HLA-C"}))
	{
		QByteArray normal_hla_allel1 = normal_hla.isValid() ? normal_hla.getGeneAllele(gene, true)  : "nicht bestimmbar";
		QByteArray normal_hla_allel2 = normal_hla.isValid() ? normal_hla.getGeneAllele(gene, false) : "nicht bestimmbar";

		QByteArray tumor_hla_allel1 = tumor_hla.isValid() ? tumor_hla.getGeneAllele(gene, true)  : "nicht bestimmbar";
		QByteArray tumor_hla_allel2 = tumor_hla.isValid() ? tumor_hla.getGeneAllele(gene, false) : "nicht bestimmbar";


		table.addDataRow({gene, normal_hla_allel1, normal_hla_allel2, tumor_hla_allel1, tumor_hla_allel2}, {1522, 2100, 2100, 2100, 2100});
	}

	table.setUniqueBorder(1,"brdrhair",4);
	return table;
}

RtfTable SomaticReportHelper::signatureTable()
{
	// load descriptions from file:
	QSharedPointer<VersatileFile> desc_file = Helper::openVersatileFileForReading(":/resources/signature_description.tsv");

	QMap<QByteArray, QByteArray> descriptions;

	while (! desc_file->atEnd())
	{
		QByteArray line = desc_file->readLine();
		line = line.trimmed();
		if (line.startsWith('#') || line.isEmpty()) continue;

		QByteArrayList parts = line.split('\t');
		if (parts.count() != 2)
		{
			THROW(FileParseException, "Signature description file has a line with more or less than 2 elements: " + line);
		}

		descriptions.insert(parts[0], parts[1]);
	}

	RtfTable table;

	QList<int> cell_widths = {1500, 1500, 1500, 2000, 3422};
	table.addRow(RtfTableRow("Mutationssignaturen", doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4));
	table.addRow(RtfTableRow({"Signatur", "Anteil [%]", "Korrelation", "Kosinus-Ähnlichkeit", "Aetiologie"}, cell_widths, RtfParagraph().setBold(true).setHorizontalAlignment("c")));

	signatureTableHelper(table, settings_.sbs_signature, descriptions, "SBS92");
	signatureTableHelper(table, settings_.id_signature, descriptions, "ID83");
	signatureTableHelper(table, settings_.dbs_signature, descriptions, "DBS78");
	signatureTableHelper(table, settings_.cnv_signature, descriptions, "CNV48");

	table.setUniqueBorder(1,"brdrhair",4);

	//Add table description
	RtfSourceCode desc = "";
	desc += RtfText("Beschreibung: ").setBold(true).setFontSize(14).RtfCode();
	desc += RtfText("SBS").setFontSize(14).setBold(true).RtfCode() + " single base substitution Signatur, ";
	desc += RtfText("ID").setFontSize(14).setBold(true).RtfCode() + " small insertions and deletions Signatur, ";
	desc += RtfText("DBS").setFontSize(14).setBold(true).RtfCode() + " doublet base substitution Signatur, ";
	desc += RtfText("CN").setFontSize(14).setBold(true).RtfCode() + " copy number Signatur, ";
	desc += RtfText("Anteil").setFontSize(14).setBold(true).RtfCode() + " prozentualer Anteil der Signatur an allen extrahierten Signaturen dieses Signaturtyps, ";
	desc += RtfText("Korrelation").setFontSize(14).setBold(true).RtfCode() + " statistisches Maß für die Plausibilität der extrahierten Mutationssignatur im Vergleich zu den beobachteten somatischen Veränderungen, ";
	desc += RtfText("Kosinus-Ähnlichkeit:").setFontSize(14).setBold(true).RtfCode() + " Maß für die Ähnlichkeit zweier Vektoren der identifizierten Patienten-Signatur gegenüber den Referenzsignaturen, ";
	desc += RtfText("Aetiologie: ").setFontSize(14).setBold(true).RtfCode();
	desc += "biologischer Prozess, der mit der vorliegenden Mutationssignatur assoziiert wurde. Mutationssignaturen siehe PMID: 32025018, Kopienzahlsignaturen siehe PMID: 35705804. ";
	desc += " Nähere Informationen erhalten Sie aus der Datenbank COSMIC (https://cancer.sanger.ac.uk/signatures/).";

	table.addRow(RtfTableRow(desc,doc_.maxWidth(),RtfParagraph().setFontSize(14).setHorizontalAlignment("j")));

	return table;
}

void SomaticReportHelper::signatureTableHelper(RtfTable &table, QString file, const QMap<QByteArray, QByteArray>& descriptions, const QByteArray& type)
{
	VersatileFile stream(file);

	if (file != "" && stream.exists() && stream.open(QIODevice::ReadOnly))
	{
		QList<int> cell_widths = {1500, 1500, 1500, 2000, 3422};

		QByteArray content = stream.readAll();
		QByteArrayList lines = content.split('\n');
		if (lines.count() < 2) return;

//		QByteArray header = lines[0];
		QByteArray values = lines[1];

		QByteArrayList parts = values.split(',');

		QByteArray cos_similarity = parts[5];
		QByteArray correlation = parts[6];

		//if there is only a single resulting signature it has no percentage after it.
		if (parts[0].trimmed() == parts[1].trimmed())
		{
			RtfTableRow row;
			row.addCell(doc_.maxWidth(), "Für die Mutationssignaturen des Typs " + type + " konnten keine COSMIC Signaturen identifieziert werden.");
			table.addRow(row);
			return;
		}

		QByteArrayList signatures = (parts[1] + "&").split('&');

		foreach (auto sig, signatures)
		{
			sig = sig.trimmed();
			if (sig == "") continue;
			sig.replace("Signature ", "");

			QByteArray sig_name = sig.split(' ')[0];
			QByteArray sig_perc = sig.split(' ')[1];
			sig_perc.replace("(", "");
			sig_perc.replace("%)", "");

			RtfTableRow row;
			row.addCell(cell_widths[0], sig_name);
			row.addCell(cell_widths[1], sig_perc.replace(".", ",").trimmed());
			row.addCell(cell_widths[2], correlation.replace(".", ",").trimmed());
			row.addCell(cell_widths[3], cos_similarity.replace(".", ",").trimmed());
			row.addCell(cell_widths[4], descriptions[sig_name]);

			table.addRow(row);
		}
	}
	else
	{
		RtfTableRow row;
		row.addCell(doc_.maxWidth(), "Die Mutationssignaturen des Typs " + type + " konnten nicht berechnet werden.");
		table.addRow(row);
	}
}



void SomaticReportHelper::storeRtf(const QByteArray& out_file)
{

	/******************************
	 * GENERAL REPORT INFORMATION *
	 ******************************/
	doc_.addPart(partSummary());
	doc_.addPart(RtfParagraph("").RtfCode());

	RtfSourceCode text = "In der nachfolgenden Übersicht finden Sie alle Varianten und Kopienzahlveränderungen, die in unterschiedlichen Datenbanken als funktionell relevant eingestuft wurden. ";
	text += "Alle aufgelisteten somatischen Veränderungen sind, wenn nicht anderweitig vermerkt, im Normalgewebe nicht nachweisbar.";
	doc_.addPart(RtfParagraph(text).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());
	doc_.addPart(RtfParagraph("").setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());

	/*****************************
	 * RELEVANT SOMATIC VARIANTS *
	 *****************************/
	doc_.addPart( partRelevantVariants() );
	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setFontSize(18).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).RtfCode());

	/*********************
	 * ADDITIONAL REPORT *
	 *********************/
	doc_.newPage();

	doc_.addPart(partUnclearVariants());
	doc_.addPart(RtfParagraph("").RtfCode());

	low_impact_indices_converted_to_high_.clear();

	doc_.addPart(partCnvTable());
	doc_.addPart(RtfParagraph("").RtfCode());

	/***********
	 * FUSIONS *
	 ***********/
	doc_.addPart(partFusions());
	doc_.addPart(RtfParagraph("").RtfCode());

	/***************
	 * VIRUS TABLE *
	 ***************/
	if(validated_viruses_.count() > 0)
	{
		doc_.addPart(partVirusTable());
		doc_.addPart(RtfParagraph("").RtfCode());
	}

	/**************************
	 * PHARMACOGENOMICS TABLE *
	 **************************/
	doc_.addPart(partPharmacoGenetics());
	doc_.addPart(RtfParagraph("").RtfCode());

	/*******************
	 * PATHWAY SUMMARY *
	 *******************/

	doc_.newPage();
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.addPart(partPathways());
	doc_.addPart(RtfParagraph("").RtfCode());

	/*******************
	 * SIGNATURE TABLE *
	 *******************/

	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.addPart(signatureTable().RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());

	/*******************************************
	 * GENERAL INFORMATION / QUALITY PARAMETERS*
	 *******************************************/
	doc_.addPart(partMetaData());
	doc_.addPart(RtfParagraph("").RtfCode());

	/******************
	 * IGV SCREENSHOT *
	 ******************/
	if(!settings_.igv_snapshot_png_hex_image.isEmpty())
	{
		doc_.addPart(partIgvScreenshot());
		doc_.addPart(RtfParagraph("").RtfCode());
	}

	/*************
	 * HLA Table *
	 *************/

	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.addPart(hlaTable(settings_.tumor_ps, settings_.normal_ps).RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());

	/***********************
	 * BILLING INFORMATION *
	 ***********************/

	doc_.newPage();
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.addPart(partBillingTable());
	doc_.addPart(RtfParagraph("").RtfCode());

	doc_.save(out_file);
}

void SomaticReportHelper::storeXML(QString file_name)
{
	VariantList som_var_in_normal = SomaticReportSettings::filterGermlineVariants(germline_vl_, settings_);
	SomaticXmlReportGeneratorData data = getXmlData(som_var_in_normal);

	QSharedPointer<QFile> out_file = Helper::openFileForWriting(file_name);
	SomaticXmlReportGenerator::generateXML(data, out_file, db_, false);
	out_file->close();
}

SomaticXmlReportGeneratorData SomaticReportHelper::getXmlData(const VariantList& som_var_in_normal)
{
	SomaticXmlReportGeneratorData data(build_, settings_, somatic_vl_, som_var_in_normal, cnvs_);

	data.tumor_content_histology = histol_tumor_fraction_ / 100.0; //is stored as double between 0 and 1, NGSD contains percentages
	data.tumor_content_snvs = getTumorContentBySNVs() / 100.0; //is stored as a double between 0 and 1, QCML file contains percentages
	data.tumor_content_clonality = getCnvMaxTumorClonality(cnvs_) ;
	data.tumor_mutation_burden = mutation_burden_;
	data.mantis_msi = mantis_msi_swd_value_;

	RtfDocument doc;
	addColors(doc);
	data.rtf_part_header = doc.header();
	data.rtf_part_footer = doc.footer();

	data.rtf_part_summary = partSummary();
	data.rtf_part_relevant_variants = partRelevantVariants();
	data.rtf_part_unclear_variants = partUnclearVariants();

	low_impact_indices_converted_to_high_.clear();

	data.rtf_part_cnvs = partCnvTable();
	data.rtf_part_svs = partFusions();
	data.rtf_part_pharmacogenetics = partPharmacoGenetics();
	data.rtf_part_general_info = partMetaData();
	data.rtf_part_igv_screenshot = partIgvScreenshot();
	data.rtf_part_mtb_summary = partPathways();
	data.rtf_part_hla_summary = hlaTable(settings_.tumor_ps, settings_.normal_ps).RtfCode();

	return data;
}

void SomaticReportHelper::storeQbicData(QString path)
{
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
	en2de["tumor cell content too low"] = "Tumorzellgehalt niedrig";
	en2de["quality of tumor DNA too low"] = "Qualität der Tumor-DNA zu gering";
	en2de["DNA quantity too low"] = "DNA-Menge im Tumor zu gering";
	en2de["heterogeneous sample"] = "Probe heterogen";
	en2de["contamination"] = "Hinweise auf Fremd-DNA";
	en2de["activating"] = "aktivierend";
	en2de["test_dependent"] = "testabhängige Bedeutung";
	en2de["ONCOGENIC"] = "onkogene Variante";
	en2de["LIKELY_ONCOGENIC"] = "wahrsch. onkogene Variante";
	en2de["BENIGN"] = "gutartige Variante";
	en2de["LIKELY_BENIGN"] = "wahrsch. gutartige Variante";
	en2de["UNCERTAIN_SIGNIFICANCE"] = "unklare Variante";
	en2de["loss_of_function"] = "Funktionsverlust";
	en2de["ambiguous"] = "unklare Bedeutung";
	en2de["proof"] = "Hinweise auf eine HRD";
	en2de["no proof"] = "Keine Hinweise auf eine HRD";
	en2de["undeterminable"] = "nicht bestimmbar";


	if(!en2de.contains(text)) return text; //return original entry if not found

	return en2de[text];
}

RtfSourceCode SomaticReportHelper::partSummary()
{
	/************
	 * METADATA *
	 ************/

	RtfTable general_info_table;
	general_info_table.addRow(RtfTableRow("Allgemeine genetische Charakteristika (" + RtfText(settings_.tumor_ps.toUtf8() + "-" + settings_.normal_ps.toUtf8()).setFontSize(16).setBold(false).RtfCode() +")",doc_.maxWidth(),RtfParagraph().setHorizontalAlignment("c").setBold(true)).setBackgroundColor(4).setBorders(1,"brdrhair",4));

	QByteArray tumor_content_bioinf = "";
	if(settings_.report_config.includeTumContentByClonality()) tumor_content_bioinf = QByteArray::number(getCnvMaxTumorClonality(cnvs_) * 100., 'f', 0) + " \%";
	if(settings_.report_config.includeTumContentByMaxSNV())
	{
		double tumor_molecular_proportion = getTumorContentBySNVs();
		if(tumor_content_bioinf != "") tumor_content_bioinf += ", ";

		tumor_content_bioinf += QByteArray::number(tumor_molecular_proportion, 'f', 1) + " \%";
	}

	if(!settings_.report_config.includeTumContentByClonality() && !settings_.report_config.includeTumContentByMaxSNV())
	{
		tumor_content_bioinf = "nicht bestimmbar";
	}

	//estimated tumor content (if checked overwrite bioinf tumor content)
	if(settings_.report_config.includeTumContentByEstimated())
	{
		tumor_content_bioinf = "ca. " + QByteArray::number(settings_.report_config.tumContentByEstimated(), 'f', 0) + " \%";
	}


	QByteArray tumor_content_hist = "nicht bestimmbar";
	if(settings_.report_config.includeTumContentByHistological())
	{
		tumor_content_hist = QByteArray::number(histol_tumor_fraction_, 'f', 0) + " \%";
	}
	general_info_table.addRow(RtfTableRow({"Tumoranteil (hist./molekular)", tumor_content_hist + " / " + tumor_content_bioinf}, {2500, 7421}).setBorders(1,"brdrhair", 4));


	RtfText mutation_burden_text;
	if (settings_.report_config.includeMutationBurden())
	{
		mutation_burden_text.setContent(QByteArray::number(mutation_burden_).replace(".", ",") + " Var/Mbp");
		if(settings_.report_config.tmbReferenceText() != "")
		{
			mutation_burden_text.append(";");
			mutation_burden_text.append(RtfText("Vergleichswerte: " + settings_.report_config.tmbReferenceText().toUtf8()).setFontSize(14).RtfCode(), true);
		}
	}
	else
	{
		mutation_burden_text.setContent("nicht bestimmbar");
	}

	general_info_table.addRow(RtfTableRow({"Mutationslast", mutation_burden_text.RtfCode()}, {2500,7421}).setBorders(1,"brdrhair",4));
	general_info_table.last()[0].setBorder(1,1,1,0,"brdrhair");
	general_info_table.last().last().setBorder(1,1,1,0,"brdrhair");


	//MSI status, values larger than 0.16 are considered unstable
	if(settings_.report_config.msiStatus())
	{
		if(processing_system_data_.type == "WES")
		{
			general_info_table.addRow(RtfTableRow({"Mikrosatelliten", ( mantis_msi_swd_value_ <= 0.3 ? "kein Hinweis auf eine MSI" : "Hinweise auf MSI" ) },{2500,7421}).setBorders(1,"brdrhair",4));
		}
		else
		{
			general_info_table.addRow(RtfTableRow({"Mikrosatelliten", ( mantis_msi_swd_value_ <= 0.16 ? "kein Hinweis auf eine MSI" : "Hinweise auf MSI" ) },{2500,7421}).setBorders(1,"brdrhair",4));
		}
	}
	else
	{
		general_info_table.addRow(RtfTableRow({"Mikrosatelliten", "nicht bestimmbar" },{2500,7421}).setBorders(1,"brdrhair",4));
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
	foreach(const auto& virus, validated_viruses_)
	{
		if (virus_names.contains(virus.virusName())) continue;
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

		general_info_table.addRow(RtfTableRow({"CNV-Last", text_cnv_burden},{2500,7421},RtfParagraph()).setBorders(1,"brdrhair",4));
	}


	RtfSourceCode hrd_text = trans(settings_.report_config.hrdStatement()).toUtf8();
	if(settings_.report_config.hrdStatement() != "undeterminable")
	{
		hrd_text += RtfText("\n\\line\nHRD-Score chromosomale Veränderungen: " + QByteArray::number(settings_.report_config.cnvLohCount() + settings_.report_config.cnvTaiCount() + settings_.report_config.cnvLstCount()) + " (HRD bei \\u8805; 42)" ).setFontSize(14).RtfCode();
	}
	general_info_table.addRow(RtfTableRow({"HRD-Score", hrd_text}, {2500,7421},  RtfParagraph()).setBorders(1, "brdrhair", 4));


	if(settings_.report_config.quality().count() >= 1)
	{
		QStringList quality_comments = settings_.report_config.quality();
		if (quality_comments[0]  != "no abnormalities" && quality_comments[0].trimmed() != "")
		{
			QStringList translated;
			foreach(QString qual_comment, quality_comments)
			{
				translated.append(trans(qual_comment.toUtf8()));
			}
			general_info_table.addRow(RtfTableRow({"Anmerkungen", translated.join(", ").toUtf8()}, {2500, 7421}, RtfParagraph()).setBorders(1, "brdrhair", 4));
		}
	}

	RtfSourceCode desc = "";
	desc += RtfText("Tumoranteil").setFontSize(14).setBold(true).RtfCode() + " (hist.): Von der Pathologie mitgeteilt; (molekular): Die Berechnung des Tumoranteils beruht auf dem Anteil der nachgewiesenen SNVs oder CNVs. ";
	desc += RtfText("Mutationslast:").setFontSize(14).setBold(true).RtfCode() + " Anzahl der Varianten in den kodierenden untersuchten Genen normiert auf eine Million Basenpaare; ";
	desc += RtfText("Mikrosatelliten:").setFontSize(14).setBold(true).RtfCode() + " Bewertung der Mikrosatelliteninstabilität; ";
	desc += RtfText("CNV-Last:").setFontSize(14).setBold(true).RtfCode() + " Anteil des Genoms, bei dem die Kopienzahl verändert ist. ";
	desc += RtfText("HRD:").setFontSize(14).setBold(true).RtfCode() + " Homologe Rekombinations-Defizienz.";
	general_info_table.addRow(RtfTableRow(desc, doc_.maxWidth(), RtfParagraph().setFontSize(14).setHorizontalAlignment("j")).setBorders(0) );

	return general_info_table.RtfCode();
}

RtfSourceCode SomaticReportHelper::partFusions()
{
	RtfTable fusion_table;
	fusion_table.addRow(RtfTableRow("Strukturvarianten",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader().setBackgroundColor(4));

	if(! settings_.report_config.fusionsDetected())
	{
		fusion_table.addRow(RtfTableRow("Nicht nachgewiesen", doc_.maxWidth()));
		fusion_table.setUniqueBorder(1,"brdrhair",4);
		return fusion_table.RtfCode();
	}

	fusion_table.addRow(RtfTableRow({"Variante", "Genomische Bruchpunkte", "Beschreibung"}, {1700, 3000, 5221}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());
	fusion_table.addRow(RtfTableRow({"", "", "", ""}, {1700,1500,1500,5221}, RtfParagraph().setFontSize(16)));
	fusion_table.setUniqueBorder(1,"brdrhair",4);

	return fusion_table.RtfCode();
}

RtfSourceCode SomaticReportHelper::partUnclearVariants()
{
	RtfSourceCode out;
	out.append(RtfParagraph("Varianten unklarer Onkogenität:").setBold(true).setIndent(0,0,0).setSpaceBefore(250).RtfCode());
	out.append(snvTable(somatic_vl_low_impact_indices_, false).RtfCode());
	return out;
}


RtfSourceCode SomaticReportHelper::partRelevantVariants()
{
	QList<RtfSourceCode> out;

	//Write hint in case of unclassified variants
	bool unclassified_snvs = false;
	int i_som_vicc = somatic_vl_.annotationIndexByName("NGSD_som_vicc_interpretation");
	for(int i=0; i<somatic_vl_.count(); ++i)
	{
		if(somatic_vl_[i].annotations()[i_som_vicc].trimmed().isEmpty())
		{
			unclassified_snvs = true;
			break;
		}
	}

	if(unclassified_snvs)
	{
		out << RtfParagraph("In der Tumorprobe wurde eine hohe Zahl somatischer Veränderungen nachgewiesen. Eine Variantenbewertung erfolgte für bekannte Treiber. Weitere Varianten werden im Anhang gelistet. Auf Wunsch kann die Bewertung ausgewählter Varianten aus dieser Liste ergänzt werden. Bitte nehmen Sie hierfür bei Bedarf Kontakt mit uns auf.").setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).highlight(3).RtfCode();
		out << RtfParagraph("").setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode();
	}

	out << RtfParagraph("Potentiell relevante somatische Veränderungen:").setBold(true).setIndent(0,0,0).setSpaceBefore(250).RtfCode();
	RtfTable high_significant_table = snvTable(somatic_vl_high_impact_indices_, true);
	out << high_significant_table.RtfCode();
	out << RtfParagraph("").RtfCode();

	if(skipped_amp_.count() > 0)
	{
		RtfSourceCode text = "Weiterhin wurden nicht-fokale Amplifikationen (3 Kopien) für die folgenden relevanten Gene nachgewiesen: ";
		std::sort(skipped_amp_.begin(), skipped_amp_.end());
		text += RtfText(skipped_amp_.join(", ")).setFontSize(18).setItalic(true).RtfCode();
		text += ".";
		out << RtfParagraph(text).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode();
		out << RtfParagraph("").setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setFontSize(18).RtfCode();
	}

	if(settings_.report_config.fusionsDetected())
	{
		RtfSourceCode snv_expl = "Es gibt Hinweise auf eine Deletion/Fusion/Translokation/Strukturvariante, die zu einer Fusion/Deletion/... führen könnte (s. Anlage).";
		out << RtfParagraph(snv_expl).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).setBold(true).highlight(3).RtfCode();
		out << RtfParagraph("").setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setFontSize(18).RtfCode();
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
	out << RtfParagraph(snv_expl).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setHorizontalAlignment("j").RtfCode();

	return out.join("\n");
}

struct PathwaysEntry
{
	QByteArray gene;
	QByteArray alteration;
	bool highlight = false;
};

RtfSourceCode SomaticReportHelper::partPathways()
{
	int i_som_rep_alt = somatic_vl_.annotationIndexByName("alt_var_alteration", true, false);
	QByteArrayList pathways = db_.getSomaticPathways();

	//create table
	RtfTable table;

	QByteArray heading_text = "Informationen zu molekularen Signalwegen";
	table.addRow(RtfTableRow(heading_text,doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(4).setHeader());

	int germline_index_coding_splicing = germline_vl_.annotationIndexByName("coding_and_splicing");
	int germline_index_class = germline_vl_.annotationIndexByName("classification");
	VariantList som_var_in_normal = SomaticReportSettings::filterGermlineVariants(germline_vl_, settings_);

	for (int i=0; i<pathways.count(); i+=4)
	{
		QByteArrayList headers;
		QByteArrayList contents;
		for (int j=i; j<i+4; ++j)
		{
			if (j<pathways.count())
			{
				//header
				QByteArray pathway = pathways[j];
				headers << pathway;

				//determine entries for small variants
				QList<PathwaysEntry> entries;
				GeneSet genes_pathway = db_.getSomaticPathwayGenes(pathway);

				//germline Variants
				for (int g=0; g<som_var_in_normal.count(); ++g)
				{
					const Variant& variant = som_var_in_normal[g];
					VariantTranscript transcript = selectSomaticTranscript(variant, settings_, germline_index_coding_splicing);
					if (genes_pathway.contains(transcript.gene))
					{
						QByteArray variant_text;
						if(!transcript.hgvs_p.trimmed().isEmpty())
						{
							variant_text = transcript.hgvs_p;
						}
						else if(!transcript.hgvs_c.trimmed().isEmpty())
						{
							variant_text = transcript.hgvs_c;
						}

						PathwaysEntry entry;
						entry.gene = transcript.gene;
						entry.alteration = variant_text.isEmpty() ?  RtfText("???").highlight(3).RtfCode() : variant_text;
						entry.highlight = (variant.annotations()[germline_index_class] == "4" || variant.annotations()[germline_index_class] == "5"); // should always be the case only class 4 and 5 germline variants are reported.
						entries.append(entry);
					}
				}

				//somatic Variants
				for(int v=0; v<somatic_vl_.count(); ++v)
				{
					const Variant& variant = somatic_vl_[v];
					VariantTranscript transcript = selectSomaticTranscript(variant, settings_, snv_index_coding_splicing_);
					if (genes_pathway.contains(transcript.gene))
					{
						QByteArray variant_text;
						if (i_som_rep_alt > -1 && !variant.annotations().at(i_som_rep_alt).trimmed().isEmpty())
						{
							variant_text = variant.annotations()[i_som_rep_alt];
						}
						else if(!transcript.hgvs_p.trimmed().isEmpty())
						{
							variant_text = transcript.hgvs_p;
						}
						else if(!transcript.hgvs_c.trimmed().isEmpty())
						{
							variant_text = transcript.hgvs_c;
						}

						PathwaysEntry entry;
						entry.gene = transcript.gene;
						entry.alteration = variant_text.isEmpty() ?  RtfText("???").highlight(3).RtfCode() : variant_text;
						entry.highlight = somatic_vl_high_impact_indices_.contains(v);
						entries.append(entry);
					}
				}

				//determine entries for CNVs (del/dup, amp, ...)
				for(int i=0;i<cnvs_.count();++i)
				{
					if (!cnv_high_impact_indices_.contains(i)) continue;

					const CopyNumberVariant& cnv = cnvs_[i];

					int cn = cnv.copyNumber(cnvs_.annotationHeaders());

					GeneSet genes_cnv = db_.genesOverlapping(cnv.chr(), cnv.start(), cnv.end());
					foreach(const QByteArray& gene, genes_cnv)
					{
						if (!genes_pathway.contains(gene)) continue;
						if (!cnv_high_impact_indices_[i].contains(gene)) continue;

						PathwaysEntry entry;
						entry.gene = gene;
						entry.alteration = CnvTypeDescription(cn, true);
						entry.highlight = true;
						entries.append(entry);
					}
				}

				//add entries to content table cell
				QByteArrayList rtf_text;
				foreach(const PathwaysEntry& entry, entries)
				{
					QByteArray text = RtfText(entry.gene).setFontSize(18).RtfCode() + " " + RtfText(entry.alteration).setFontSize(16).RtfCode();
					if (!entry.highlight) text = RtfText("[ ").setFontSize(18).RtfCode() + text + RtfText(" ]").setFontSize(18).RtfCode();
					rtf_text << text;
				}
				contents << rtf_text.join("\\line\n");
			}
			else
			{
				headers << "";
				contents << "";
			}
		}
		table.addRow(RtfTableRow(headers,{2480,2480,2480,2480},RtfParagraph().setHorizontalAlignment("c").setBold(true).setItalic(true)).setBorders(1,"brdrhair",4).setBackgroundColor(5));
		table.addRow(RtfTableRow(contents,{2480,2480,2480,2480},RtfParagraph().setHorizontalAlignment("c").setLineSpacing(276)).setBorders(1,"brdrhair",4));
	}

	//add description below table
	RtfSourceCode desc = "";
	desc += RtfText("Beschreibung: ").setFontSize(14).setBold(true).RtfCode();
	desc += "Die nachgewiesenen potentiell relevanten somatischen Veränderungen und die unklaren Varianten (in eckigen Klammern) wurden nach den wichtigsten molekularen Signalwegen sortiert. Die Zugehörigkeit eines Gens zu einem bestimmten Signalweg wurde durch das Molekulare Tumorboard Tübingen festgestellt.";
	table.addRow(RtfTableRow(desc,doc_.maxWidth(),RtfParagraph().setFontSize(14).setHorizontalAlignment("j")));

	return table.RtfCode();
}

QByteArray SomaticReportHelper::prepareTranscriptType(QByteArray transcript_type)
{
	if (! transcript_type.contains(","))
	{
		return transcript_type;
	}

	QByteArray clean_transcript_type;
	foreach(QByteArray t, transcript_type.split(','))
	{
		t = t.trimmed();
		if (t != "intron")
		{
			clean_transcript_type += t +", ";
		}
	}

	clean_transcript_type.chop(2);
	return clean_transcript_type;
}

double SomaticReportHelper::getTumorContentBioinf()
{
	if(settings_.report_config.includeTumContentByClonality() && settings_.report_config.includeTumContentByMaxSNV())
	{
		return std::max(getCnvMaxTumorClonality(cnvs_), getTumorContentBySNVs());
	}

	if(settings_.report_config.includeTumContentByClonality()) return getCnvMaxTumorClonality(cnvs_);
	if(settings_.report_config.includeTumContentByMaxSNV()) return getTumorContentBySNVs();

	return -1;

}

