#include "SomaticXmlReportGenerator.h"
#include "BasicStatistics.h"
#include "LoginManager.h"
#include "GenLabDB.h"
#include "XmlHelper.h"

#include "SomaticReportConfiguration.h"

#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

SomaticXmlReportGeneratorData::SomaticXmlReportGeneratorData(GenomeBuild genome_build, const SomaticReportSettings &som_settings, const VariantList& snvs, const VariantList& germl_snvs, const CnvList& cnvs)
	: build(genome_build)
	, settings(som_settings)
	, tumor_snvs(snvs)
	, germline_snvs(germl_snvs)
	, tumor_cnvs(cnvs)
	, tumor_content_histology(std::numeric_limits<double>::quiet_NaN())
	, tumor_content_snvs(std::numeric_limits<double>::quiet_NaN())
	, tumor_content_clonality(std::numeric_limits<double>::quiet_NaN())
	, tumor_mutation_burden(std::numeric_limits<double>::quiet_NaN())
	, mantis_msi(std::numeric_limits<double>::quiet_NaN())
{
}

void SomaticXmlReportGeneratorData::check() const
{
	QStringList messages;

	if( settings.report_config.tumContentByHistological() && !BasicStatistics::isValidFloat(tumor_content_histology))
	{
		messages << "Tumor content by histology selected but value is not valid float";
	}

	if( settings.report_config.tumContentByMaxSNV() && !BasicStatistics::isValidFloat(tumor_content_snvs))
	{
		messages << "Tumor content by median SNV B-AF selected but value is not valid float";
	}

	if( settings.report_config.tumContentByClonality() && !BasicStatistics::isValidFloat(tumor_content_clonality) )
	{
		messages << "Tumor content by maximum CNV clonality selected but value is not valid float";
	}

	if( !BasicStatistics::isValidFloat(tumor_mutation_burden))
	{
		messages << "Tumor mutation burden is not a valid float";
	}
	if( settings.report_config.msiStatus() && !BasicStatistics::isValidFloat(mantis_msi))
	{
		messages << "MSI status selected but value is not valid float";
	}

	if(messages.count() > 0)
	{
		THROW(ArgumentException, "Invalid data in SomaticXmlReportGeneratorData! Messages: " + messages.join(",\n"));
	}

	SomaticXmlReportGenerator::checkSomaticVariantAnnotation(tumor_snvs);

}


SomaticXmlReportGenerator::SomaticXmlReportGenerator()
{

}

void SomaticXmlReportGenerator::checkSomaticVariantAnnotation(const VariantList &vl)
{
	const QByteArrayList annos = {"tumor_af","tumor_dp", "normal_af", "normal_dp", "gene", "somatic_classification", "ncg_oncogene", "ncg_tsg", "coding_and_splicing"};

	for(const auto& anno : annos)
	{
		int i_anno = vl.annotationIndexByName(anno, true, false);
		if(i_anno < 0)
		{
			THROW(ArgumentException, "Could not find all neccessary annotations in somatic SNV file for XML generation in SomaticXmlReportGenerator::checkSomaticVariantAnnotation");
		}
	}
}

void SomaticXmlReportGenerator::generateXML(const SomaticXmlReportGeneratorData &data, QSharedPointer<QFile> out_file, NGSD& db, bool test)
{
	QString tumor_ps_id = db.processedSampleId(data.settings.tumor_ps);
	QString normal_ps_id = db.processedSampleId(data.settings.normal_ps);

	QXmlStreamWriter w(out_file.data());

	w.setAutoFormatting(true);


	w.writeStartDocument();

	//Element SomaticNgsReport
	w.writeStartElement("SomaticNgsReport");
	w.writeAttribute("version", "4");
	w.writeAttribute("genome_build", buildToString(data.build, true));

	//Element ReportGeneration
	w.writeStartElement("ReportGeneration");
	w.writeAttribute("date", (test ? "2000-01-01" : QDate::currentDate().toString("yyyy-MM-dd") ) );
	w.writeAttribute("user_name", LoginManager::userLogin());
	w.writeAttribute("software", QCoreApplication::applicationName()+ " " + QCoreApplication::applicationVersion());
	w.writeEndElement();

	//Element PatientInfo
	w.writeStartElement("PatientInfo");

		if(test)
		{
			w.writeAttribute("sap_patient_identifier", "SAP_TEST_IDENTIFIER");
		}
		else
		{
			GenLabDB genlab;
			w.writeAttribute("sap_patient_identifier", genlab.sapID(data.settings.tumor_ps) );
		}

		QList<SampleDiseaseInfo> disease_infos = db.getSampleDiseaseInfo(db.sampleId(data.settings.tumor_ps), "ICD10 code");
		if(!disease_infos.empty())
		{
			for(const auto& disease_info : disease_infos)
			{
				w.writeStartElement("DiseaseInfo");
				w.writeAttribute("type", "ICD10");
				w.writeAttribute("identifier", disease_info.disease_info);
				w.writeEndElement();
			}
		}

	w.writeEndElement();

	//Element TumorSample
	w.writeStartElement("TumorSample");
	w.writeAttribute("name", data.settings.tumor_ps);
	ProcessedSampleData t_ps_data = db.getProcessedSampleData(tumor_ps_id);
	w.writeAttribute("processing_system", t_ps_data.processing_system);
	w.writeAttribute("processing_system_type", t_ps_data.processing_system_type);
	w.writeAttribute("sequencer", db.getValue("SELECT d.type FROM device as d, sequencing_run as sr WHERE d.id = sr.device_id AND sr.name = '" + t_ps_data.run_name +  "'", false).toString());
	QCCollection t_qc = db.getQCData(tumor_ps_id);
	w.writeAttribute("average_depth", t_qc.value("QC:2000025", true).asString() );
	if( data.settings.report_config.tumContentByHistological())
	{
		w.writeAttribute("tumor_content_histology", QByteArray::number(data.tumor_content_histology, 'f', 3) );
	}
	if( data.settings.report_config.tumContentByClonality() && BasicStatistics::isValidFloat(data.tumor_content_clonality) )
	{
		w.writeAttribute("tumor_content_bioinformatic",  QString::number(data.tumor_content_clonality, 'f', 3));
	}
	else if( data.settings.report_config.tumContentByMaxSNV() && BasicStatistics::isValidFloat(data.tumor_content_snvs) )
	{
		w.writeAttribute("tumor_content_bioinformatic",  QString::number(data.tumor_content_snvs, 'f', 3));
	}
	w.writeAttribute( "mutation_burden", QString::number(data.tumor_mutation_burden,'f', 2) );
	if( data.settings.report_config.msiStatus() ) w.writeAttribute( "microsatellite_instability",  QString::number(data.mantis_msi, 'f', 2) );
	w.writeAttribute("hrd_score", QString::number(data.settings.report_config.hrdScore()) );
	w.writeAttribute("hrd_score_chromo", QString::number(data.settings.report_config.cnvLohCount() + data.settings.report_config.cnvTaiCount() + data.settings.report_config.cnvLstCount()));

	//QC data
	QCCollection qc_data = db.getQCData(tumor_ps_id);
	for (int i=0; i<qc_data.count(); ++i)
	{
		const QCValue& term = qc_data[i];
		if (term.type()==QVariant::ByteArray) continue; //skip plots
		w.writeStartElement("QcTerm");
		w.writeAttribute("id", term.accession());
		w.writeAttribute("name", term.name());
		w.writeAttribute("def", term.description());
		w.writeAttribute("value", term.toString());
		w.writeEndElement();
	}

	w.writeEndElement();


	//Element NormalSample
	w.writeStartElement("NormalSample");
	w.writeAttribute("name", data.settings.normal_ps);
	ProcessedSampleData n_ps_data = db.getProcessedSampleData(normal_ps_id);
	w.writeAttribute("processing_system", n_ps_data.processing_system);
	w.writeAttribute("processing_system_type", n_ps_data.processing_system_type);
	w.writeAttribute("sequencer", db.getValue("SELECT d.type FROM device as d, sequencing_run as sr WHERE d.id = sr.device_id AND sr.name = '" + n_ps_data.run_name +  "'", false).toString() );
	QCCollection n_qc = db.getQCData(normal_ps_id);
	w.writeAttribute("average_depth", n_qc.value("QC:2000025", true).asString() );

	//QC data
	qc_data = db.getQCData(normal_ps_id);
	for (int i=0; i<qc_data.count(); ++i)
	{
		const QCValue& term = qc_data[i];
		if (term.type()==QVariant::ByteArray) continue; //skip plots
		w.writeStartElement("QcTerm");
		w.writeAttribute("id", term.accession());
		w.writeAttribute("name", term.name());
		w.writeAttribute("def", term.description());
		w.writeAttribute("value", term.toString());
		w.writeEndElement();
	}

	w.writeEndElement();


	//Element AnalysisPipeline
	w.writeStartElement("AnalysisPipeline");
	w.writeAttribute("name", "megSAP");



	w.writeAttribute("version", data.tumor_snvs.getPipeline().replace("megSAP","").trimmed());

	w.writeAttribute("url", "https://github.com/imgag/megSAP");
	w.writeAttribute("comment", "Mapping: bwa mem, Indel Realignment: Abra2, Variant Caller: Strelka2, CNV Caller: ClinCNV");

	w.writeEndElement();

	//Element TargetRegion
	w.writeStartElement("TargetRegion");

	if(!data.settings.target_region_filter.isValid())
	{
		int sys_id = db.processingSystemIdFromProcessedSample(data.settings.tumor_ps);
		w.writeAttribute("name",  db.getProcessingSystemData(sys_id).name); //in our workflow identical to processing system name
	}
	else
	{
		w.writeAttribute("name",  data.settings.target_region_filter.name); //sub panel target has been selected
	}

	for(int i=0; i<data.settings.target_region_filter.regions.count(); ++i)
	{
		const BedLine& line = data.settings.target_region_filter.regions[i];

		w.writeStartElement("Region");
		w.writeAttribute("chr", line.chr().strNormalized(true));
		w.writeAttribute("start", QString::number(line.start()));
		w.writeAttribute("end", QString::number(line.end()));
		w.writeEndElement();
	}

	foreach(const QByteArray& gene, data.settings.target_region_filter.genes)
	{
		GeneInfo gene_info = db.geneInfo(gene);
		if(gene_info.symbol.isEmpty()) continue;
		if(gene_info.hgnc_id.isEmpty()) continue;

		w.writeStartElement("Gene");

		w.writeAttribute("name", gene_info.symbol);
		w.writeAttribute("id", gene_info.hgnc_id);
		w.writeEndElement();
	}

	//End Element TargetRegion
	w.writeEndElement();

	w.writeStartElement("VariantList");


		int i_tumor_af = data.tumor_snvs.annotationIndexByName("tumor_af",true,true);
		int i_tumor_depth = data.tumor_snvs.annotationIndexByName("tumor_dp",true,true);

		int i_normal_af = data.tumor_snvs.annotationIndexByName("normal_af", true,true);
		int i_normal_depth = data.tumor_snvs.annotationIndexByName("normal_dp", true, true);

		int i_genes = data.tumor_snvs.annotationIndexByName("gene");

		int i_ncg_oncogene = data.tumor_snvs.annotationIndexByName("ncg_oncogene");
		int i_ncg_tsg = data.tumor_snvs.annotationIndexByName("ncg_tsg");
		int i_co_sp = data.tumor_snvs.annotationIndexByName("coding_and_splicing");

		for(int i=0; i<data.tumor_snvs.count(); ++i) //variants only in tumor
		{
			const Variant& snv = data.tumor_snvs[i];

			w.writeStartElement("Variant");

			w.writeAttribute( "chr", snv.chr().strNormalized(true) );
			w.writeAttribute( "start", QString::number(snv.start()) );
			w.writeAttribute( "end", QString::number(snv.end()) );
			w.writeAttribute( "ref", snv.ref() );
			w.writeAttribute( "obs", snv.obs() );
			w.writeAttribute( "af_tumor", QString(snv.annotations()[i_tumor_af]) );
			w.writeAttribute( "depth_tumor", QString(snv.annotations()[i_tumor_depth]) );
			w.writeAttribute( "af_normal", QString(snv.annotations()[i_normal_af]) );
			w.writeAttribute( "depth_normal", QString(snv.annotations()[i_normal_depth]) );

			if(db.getSomaticViccId(snv) != -1)
			{
				w.writeAttribute( "effect", SomaticVariantInterpreter::viccScoreAsString(db.getSomaticViccData(snv)).toLower() );
			}


				QByteArrayList genes = snv.annotations()[i_genes].split(',');
				QByteArrayList oncogenes = snv.annotations()[i_ncg_oncogene].split(',');
				QByteArrayList tsg = snv.annotations()[i_ncg_tsg].split(',');

				for(int j=0; j < genes.count(); ++j)
				{

					GeneInfo gene_info = db.geneInfo(genes[j]);
					if(gene_info.symbol.isEmpty()) continue;
					if(gene_info.hgnc_id.isEmpty()) continue; //genes that have been withdrawn or cannot be mapped to a unique approved symbol
					w.writeStartElement("Gene");


					w.writeAttribute("name", gene_info.symbol);
					w.writeAttribute("id", gene_info.hgnc_id);

					SomaticGeneRole gene_role = db.getSomaticGeneRole(gene_info.symbol.toUtf8());
					if(gene_role.isValid())
					{
						w.writeAttribute("role", gene_role.asString());
					}

					if(tsg[j].contains("1"))
					{
						w.writeStartElement("IsTumorSuppressor");
						w.writeAttribute("source", "Network of Cancer Genes");
						w.writeAttribute("source_version", "6.0");
						w.writeEndElement();
					}

					if(oncogenes[j].contains("1"))
					{
						w.writeStartElement("IsOncoGene");
						w.writeAttribute("source", "Network of Cancer Genes");
						w.writeAttribute("source_version", "6.0");
						w.writeEndElement();
					}

					w.writeEndElement();
				}


				//Elements transcript information
				bool is_first = true;
				for(const auto& trans : snv.transcriptAnnotations(i_co_sp) )
				{
					w.writeStartElement("TranscriptInformation");

					w.writeAttribute("transcript_id", trans.id);
					w.writeAttribute("gene", trans.gene);
					w.writeAttribute("type", trans.type);
					w.writeAttribute("hgvs_c", trans.hgvs_c);
					w.writeAttribute("hgvs_p", trans.hgvs_p);
					w.writeAttribute("exon", trans.exon);
					w.writeAttribute("variant_type", trans.type);

					if(is_first)
					{
						w.writeAttribute("main_transcript", "true");
						is_first = false;
					}
					else
					{
						w.writeAttribute("main_transcript", "false");
					}


					w.writeEndElement();
				}

			w.writeEndElement();
		}


		int i_germl_freq_in_tum = data.germline_snvs.annotationIndexByName("freq_in_tum");
		int i_germl_depth_in_tum = data.germline_snvs.annotationIndexByName("depth_in_tum");
		int i_germl_hom_het = data.germline_snvs.annotationIndexByName(data.settings.normal_ps);
		int i_germl_co_sp = data.germline_snvs.annotationIndexByName("coding_and_splicing");

		for(int i=0; i<data.germline_snvs.count(); ++i)
		{
			const Variant& snv = data.germline_snvs[i];

			w.writeStartElement("Variant");

			w.writeAttribute( "chr", snv.chr().strNormalized(true) );
			w.writeAttribute( "start", QString::number(snv.start()) );
			w.writeAttribute( "end", QString::number(snv.end()) );
			w.writeAttribute( "ref", snv.ref() );
			w.writeAttribute( "obs", snv.obs() );
			w.writeAttribute( "af_tumor", QString(snv.annotations()[i_germl_freq_in_tum]) );
			w.writeAttribute( "depth_tumor", QString(snv.annotations()[i_germl_depth_in_tum]) );
			w.writeAttribute( "af_normal", (snv.annotations()[i_germl_hom_het].contains("het") ? "0.5" : "1.0" ) );
			if(db.getSomaticViccId(snv) != -1)
			{
				w.writeAttribute( "effect", SomaticVariantInterpreter::viccScoreAsString(db.getSomaticViccData(snv)).toLower() );
			}

			//Elements transcript information
			for(const auto& trans : snv.transcriptAnnotations(i_germl_co_sp) )
			{
				w.writeStartElement("TranscriptInformation");

				w.writeAttribute("transcript_id", trans.id);
				w.writeAttribute("gene", trans.gene);
				w.writeAttribute("type", trans.type);
				w.writeAttribute("hgvs_c", trans.hgvs_c);
				w.writeAttribute("hgvs_p", trans.hgvs_p);
				w.writeAttribute("exon", trans.exon);
				w.writeAttribute("variant_type", trans.type);

				bool is_main_transcript = data.settings.preferred_transcripts.contains(trans.gene) && data.settings.preferred_transcripts.value(trans.gene).contains(trans.idWithoutVersion());
				w.writeAttribute("main_transcript", is_main_transcript ? "true" : "false");

				w.writeEndElement();
			}

			w.writeEndElement();
		}

	w.writeEndElement();


	//Element CNVList
	w.writeStartElement("CnvList");

		//Elements CNV
	if(data.tumor_cnvs.count() > 0)
	{

		int i_clonality = data.tumor_cnvs.annotationIndexByName("tumor_clonality", true);
		int i_state = data.tumor_cnvs.annotationIndexByName("state", true); //AMP/DEL/LOH
		int i_type = data.tumor_cnvs.annotationIndexByName("cnv_type", true); //p-arm/q-arm/focal/chromosome
		int i_tumor_cn_change = data.tumor_cnvs.annotationIndexByName("tumor_CN_change", true);

		int i_cn_minor = data.tumor_cnvs.annotationIndexByName("minor_CN_allele", true);
		int i_cn_major = data.tumor_cnvs.annotationIndexByName("major_CN_allele", true);

		int i_tsg = data.tumor_cnvs.annotationIndexByName("ncg_tsg", true);
		int i_oncogene = data.tumor_cnvs.annotationIndexByName("ncg_oncogene", true);

		int i_cytoband = data.tumor_cnvs.annotationIndexByName("cytoband", true);


		for(int i=0; i<data.tumor_cnvs.count(); ++i)
		{
			const CopyNumberVariant& cnv = data.tumor_cnvs[i];

			w.writeStartElement("Cnv");

			w.writeAttribute("chr", cnv.chr().strNormalized(true));
			w.writeAttribute("start", QString::number(cnv.start()) );
			w.writeAttribute("end", QString::number(cnv.end()) );

			QByteArrayList cytobands = cnv.annotations()[i_cytoband].split(',');
			w.writeAttribute( "start_band", QString(cytobands.first()) );
			w.writeAttribute( "end_band", QString(cytobands.last()) );
			w.writeAttribute( "af", QString(cnv.annotations()[i_clonality]) );

			w.writeAttribute( "alteration", QString(cnv.annotations()[i_state]) );


			w.writeAttribute( "type", QString(cnv.annotations()[i_type]) );
			w.writeAttribute( "cn", QString(cnv.annotations()[i_tumor_cn_change]) );
			w.writeAttribute( "cn_a", QString(cnv.annotations()[i_cn_minor]));
			w.writeAttribute( "cn_b", QString(cnv.annotations()[i_cn_major]));

			GeneSet genes = cnv.genes();
			GeneSet tsg = db.genesToApproved(GeneSet::createFromText( cnv.annotations()[i_tsg], ',' ), true);
			GeneSet oncogenes = db.genesToApproved(GeneSet::createFromText( cnv.annotations()[i_oncogene], ',' ), true);
			for(const auto& gene : genes)
			{
				QByteArray gene_symbol = db.geneToApproved(gene).trimmed();
				if (gene_symbol.isEmpty()) continue; //no approved symbol

				if(!data.settings.target_region_filter.genes.contains(gene)) continue; //Include genes from target filter only

				w.writeStartElement("Gene");
				w.writeAttribute("name", gene_symbol);
				w.writeAttribute("id", "HGNC:" + db.getValue("SELECT hgnc_id FROM gene WHERE symbol = '" + gene_symbol + "'").toString());

				SomaticGeneRole gene_role = db.getSomaticGeneRole(gene);
				if(gene_role.isValid())
				{
					w.writeAttribute("role",  gene_role.asString());
				}

				if(tsg.contains(gene_symbol))
				{
					w.writeStartElement("IsTumorSuppressor");
					w.writeAttribute("source", "Network of Cancer Genes");
					w.writeAttribute("source_version", "6.0");
					w.writeEndElement();
				}

				if(oncogenes.contains(gene_symbol))
				{
					w.writeStartElement("IsOncoGene");
					w.writeAttribute("source", "Network of Cancer Genes");
					w.writeAttribute("source_version", "6.0");
					w.writeEndElement();
				}

				w.writeEndElement();
			}


			w.writeEndElement();
		}
	}

	w.writeEndElement();

	//Element ReportDocument
	w.writeStartElement("ReportDocument");
	w.writeAttribute("format", "RTF");
	w.writeEndElement();

	writeReportPartsElement(w, "summary", data.rtf_part_header+data.rtf_part_summary+data.rtf_part_footer);
	writeReportPartsElement(w, "relevant_variants", data.rtf_part_header+data.rtf_part_relevant_variants+data.rtf_part_footer);
	writeReportPartsElement(w, "unclear_variants", data.rtf_part_header+data.rtf_part_unclear_variants+data.rtf_part_footer);
	writeReportPartsElement(w, "cnvs", data.rtf_part_header+data.rtf_part_cnvs+data.rtf_part_footer);
	writeReportPartsElement(w, "svs", data.rtf_part_header+data.rtf_part_svs+data.rtf_part_footer);
	writeReportPartsElement(w, "pharmaco_genetics", data.rtf_part_header+data.rtf_part_pharmacogenetics+data.rtf_part_footer);
	writeReportPartsElement(w, "general_info", data.rtf_part_header+data.rtf_part_general_info+data.rtf_part_footer);
	writeReportPartsElement(w, "igv_screenshot", data.rtf_part_header+data.rtf_part_igv_screenshot+data.rtf_part_footer);
	writeReportPartsElement(w, "mtb_summary", data.rtf_part_header+data.rtf_part_mtb_summary+data.rtf_part_footer);
	writeReportPartsElement(w, "hla_summary", data.rtf_part_header+data.rtf_part_hla_summary+data.rtf_part_footer);

	//End Element SomaticNgsReport
	w.writeEndElement();

	w.writeEndDocument();
}


void SomaticXmlReportGenerator::validateXml(QString file_name)
{
	QString xml_error = XmlHelper::isValidXml(file_name, ":/resources/SomaticReport.xsd");

	if(xml_error!= "")
	{
		THROW(ProgrammingException, "SomaticXmlReportGenerator::generateXML produced an invalid XML file: " + xml_error);
	}

}

void SomaticXmlReportGenerator::writeReportPartsElement(QXmlStreamWriter &w, QString name, RtfSourceCode rtf_part)
{
	w.writeStartElement("ReportDocumentParts");
	w.writeAttribute("name", name);
	w.writeAttribute("format", "RTF");
	w.writeCharacters(rtf_part.toBase64());
	w.writeEndElement();
}
