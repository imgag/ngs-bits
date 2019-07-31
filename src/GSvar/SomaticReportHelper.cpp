#include "SomaticReportHelper.h"
#include "BasicStatistics.h"
#include "OntologyTermCollection.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "NGSHelper.h"

#include <cmath>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMainWindow>
#include <QSqlError>

RtfTable SomaticReportHelper::somaticAlterationTable(const VariantList& snvs, const CnvList& cnvs, bool include_cnvs,const GeneSet& target_genes)
{
	RtfTable table;

	QByteArray heading_text = "Punktmutationen (SNVs), kleine Insertionen/Deletionen (INDELs)";
	if(include_cnvs) heading_text +=  " und Kopienzahlvarianten (CNVs)";
	table.addRow(RtfTableRow(heading_text,doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(5).setHeader());
	table.addRow(RtfTableRow({"Gene","Position","Type","Fraction","Description"},{1000,2400,1700,900,3638},RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader());

	if(snvs.count() == 0 && cnvs.isEmpty())
	{
		table.removeRow(1); //remove description header
		table.addRow(RtfTableRow("Es wurden keine potentiell relevanten somatischen Veränderungen nachgewiesen.",doc_.maxWidth()));
		return table;
	}

	int i_cnv_tum_clonality = cnvs.annotationIndexByName("tumor_clonality");
	if(i_cnv_tum_clonality < 0)
	{
		table.addRow(RtfTableRow("No column \"tumor_clonality\" in ClinCNV file. Please recalculate it using a more recent ClinCNV version.",doc_.maxWidth(),RtfParagraph().highlight(3)));
		return table;
	}

	for(int i=0;i<snvs.count();++i)
	{
		cgi_acronyms_.append(parse_cgi_cancer_acronyms(snvs[i].annotations().at(snv_index_cgi_driver_statement_)));
	}


	GeneSet genes_in_first_part; //cn altered genes in first part of table

	//Combined SNV and CNV table
	for(int i=0;i<snvs.count();++i)
	{
		const Variant& snv = snvs[i];

		VariantTranscript transcript = selectSomaticTranscript(snv);
		transcript.type = transcript.type.replace("_variant","");
		transcript.type.replace("&",", ");

		//Get Relevant information about cnvs that overlap snv
		int i_corresponding_cnv = -1;
		for(int j=0;j<cnvs.count();++j)
		{
			if(cnvs[j].overlapsWith(snv.chr(),snv.start(),snv.end()))
			{
				i_corresponding_cnv = j;
				break;
			}
		}

		RtfTableRow temp_snv_row;
		temp_snv_row.addCell(1000,transcript.gene,RtfParagraph().setItalic(true));
		temp_snv_row.addCell({transcript.hgvs_c + ":" + transcript.hgvs_p, RtfText(transcript.id).setFontSize(14).RtfCode()},2400);

		temp_snv_row.addCell(1700,transcript.type);
		int i_tum_af = snvs.annotationIndexByName("tumor_af",true,true);
		temp_snv_row.addCell(900,QByteArray::number(snv.annotations().at(i_tum_af).toDouble(),'f',2),RtfParagraph().setHorizontalAlignment("c")); //tumor allele frequency

		QByteArray gene_info = "NA";
		if(!snv.annotations()[snv_index_cgi_driver_statement_].isEmpty())
		{
			gene_info = snv.annotations()[snv_index_cgi_driver_statement_];
			gene_info.replace(": tier 1", "");
			gene_info.replace(": tier 2", "");
			gene_info.replace(";","; ");
		}


		gene_info.append(", " + RtfText(db_.geneInfo(transcript.gene).name.toUtf8()).setItalic(true).RtfCode());

		int i_snvs_oncogene_info = snvs.annotationIndexByName("ncg_oncogene",true,false);
		int i_snvs_tsg_info = snvs.annotationIndexByName("ncg_tsg",true,false);
		if(i_snvs_oncogene_info > -1 && i_snvs_tsg_info > -1)
		{
			if(snv.annotations().at(i_snvs_oncogene_info) == "1") gene_info.append(", oncogene");
			if(snv.annotations().at(i_snvs_tsg_info) == "1") gene_info.append(", TSG");
		}

		temp_snv_row.addCell(3638,gene_info);
		table.addRow(temp_snv_row);


		//Add row with information about CNV if there is an overlapping cnv
		if(i_corresponding_cnv > -1)
		{
			const CopyNumberVariant& cnv = cnvs[i_corresponding_cnv];
			genes_in_first_part << transcript.gene;

			//set first cell that contains gene name as cell over multiple rows
			table.last()[0].addHeaderControlWord("clvmgf");
			table.last()[4].addHeaderControlWord("clvmgf");


			RtfTableRow temp_cnv_row;
			temp_cnv_row.addCell(1000,transcript.gene,RtfParagraph().setItalic(true));


			//Add CNV information to transcript type (if cnv at this locus exists and is available)
			double tum_clonality = cnv.annotations().at(i_cnv_tum_clonality).toDouble();
			double tum_af = snv.annotations().at(i_tum_af).toDouble();
			double tum_maximum_clonality = getCnvMaxTumorClonality(cnvs);
			double tum_cn_change = cnv.annotations().at(cnv_index_tumor_cn_change_).toDouble();

			QByteArray statement = "";
			if(tum_cn_change > 2)
			{
				if(tum_af < tum_maximum_clonality/2.) statement = "AMP WT (" + cnv.annotations().at(cnv_index_tumor_cn_change_) + " copies)";
				else if(tum_af > tum_maximum_clonality/2.) statement = "AMP MUT (" + cnv.annotations().at(cnv_index_tumor_cn_change_) + " copies)";
			}
			else if(tum_cn_change < 2)
			{
				if(tum_cn_change == 1 ) statement = "DEL (het)";
				else if(tum_cn_change == 0) statement = "DEL (hom)";
			}
			else if(tum_cn_change == 2)
			{
				if(tum_af > tum_clonality/2.) statement = "LOH";
			}

			temp_cnv_row.addCell(2400,statement);

			QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_).toLower();
			if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";
			temp_cnv_row.addCell(1700,cnv_type);

			temp_cnv_row.addCell(900,QByteArray::number(cnv.annotations().at(i_cnv_tum_clonality).toDouble(),'f',2),RtfParagraph().setHorizontalAlignment("c"));
			temp_cnv_row.addCell(3638,"");
			//set first cell of corresponding cnv (contains gene name) as end of cell over multiple rows
			temp_cnv_row[0].addHeaderControlWord("clvmrg");
			temp_cnv_row[4].addHeaderControlWord("clvmrg");

			table.addRow(temp_cnv_row);
		}
	}

	if(!include_cnvs)
	{
		return table;
	}

	//Add remaining CNVs to table

	int i_ncg_oncogene = cnvs_filtered_.annotationIndexByName("ncg_oncogene");
	int i_ncg_tsg = cnvs_filtered_.annotationIndexByName("ncg_tsg");


	QMap<QByteArray,RtfTableRow> cna_genes_per_row;

	//Make list of CNV drivers
	for(int i=0;i<cnvs_filtered_.count();++i)
	{
		const CopyNumberVariant& cnv = cnvs_filtered_[i];

		QByteArrayList genes = cnv.annotations().at(cnv_index_cgi_genes_).split(',');

		QByteArrayList statements = cnv.annotations().at(cnv_index_cgi_driver_statement_).split(',');
		if(genes.count() != statements.count())
		{
			THROW(FileParseException, "Unequal number of genes and driver statements in somatic ClinCNV file in CNV " + cnv.toString());
		}
		//list with indices of driver genes
		QList<int> i_genes_to_be_printed = {};
		//Filter for drivers (HSA report)
		if(target_genes.isEmpty())
		{
			for(int j=0;j<genes.count();++j)
			{
				if(genes_in_first_part.contains(genes[j])) continue; //Skip genes that are already in first part of the report

				if(statements[j].contains("driver") || statements[j].contains("known"))
				{
					i_genes_to_be_printed << j;
				}
			}
		}
		else //Filter for target genes (EBM report)
		{
			for(int j=0;j<genes.count();++j)
			{
				if(genes_in_first_part.contains(genes[j])) continue;
				if(target_genes.contains(genes[j])) i_genes_to_be_printed << j;
			}
		}

		if(!i_genes_to_be_printed.isEmpty())
		{
			QByteArray cn_statement = getCnvType(cnv);
			QByteArray tumor_clonality = QByteArray::number(cnv.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2);

			QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_).toLower();
			if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";

			foreach(int index,i_genes_to_be_printed)
			{
				//Filter out genes that do not lie in target region
				genes[index] = db_.geneToApproved(genes[index],true);

				RtfTableRow row;
				row.addCell(1000,genes[index],RtfParagraph().setItalic(true));
				row.addCell(2400,cn_statement);
				row.addCell(1700,cnv_type);
				row.addCell(900,tumor_clonality,RtfParagraph().setHorizontalAlignment("c"));

				//Gene description
				QByteArrayList desc;
				desc << statements[index].replace(";","; ") << RtfText(db_.geneInfo(genes[index]).name.toUtf8()).setItalic(true).RtfCode();
				if(i_ncg_oncogene > -1 && i_ncg_tsg > -1)
				{
					QByteArray oncogene_statement = cnv.annotations().at(i_ncg_oncogene).split(',').at(index);
					QByteArray tsg_statement = cnv.annotations().at(i_ncg_tsg).split(',').at(index);

					if(oncogene_statement == "1") desc << "Oncogene";
					if(tsg_statement == "1") desc << "TSG";
				}

				row.addCell(3638,desc.join(", "));

				cgi_acronyms_.append(parse_cgi_cancer_acronyms(statements[index]));

				cna_genes_per_row.insert(genes[index],row);
			}
		}
	}

	//Add rows to table, sorted according gene (which is key of cna_genes_per_row QMap)
	for(const auto& row : cna_genes_per_row)
	{
		table.addRow(row);
	}

	return table;
}


RtfTable SomaticReportHelper::createCnvTable()
{
	RtfTable cnv_table;

	//Table Header
	cnv_table.addRow(RtfTableRow({"Kopienzahlvarianten (CNVs)"},doc_.maxWidth(),RtfParagraph().setHorizontalAlignment("c").setBold(true)).setBackgroundColor(5).setHeader());
	cnv_table.addRow(RtfTableRow({"Position (Typ)","CNV","CN","Anteil Probe","Gene"},{2438,962,400,700,5138},RtfParagraph().setHorizontalAlignment("c").setBold(true)).setHeader());

	RtfParagraph header_format;
	header_format.setBold(true);
	header_format.setHorizontalAlignment("c");

	if(cnvs_filtered_.isEmpty())
	{
		cnv_table.removeRow(1);
		cnv_table.addRow(RtfTableRow("Es wurden keine CNVs gefunden.",doc_.maxWidth()));
		return cnv_table;
	}

	if(cnv_index_cgi_genes_ < 0 || cnv_index_cgi_driver_statement_ < 0 || cnv_index_cgi_gene_role_ < 0)
	{
		cnv_table.addRow(RtfTableRow("Fehlerhafte CGI statements in ClinCNV-Datei.",doc_.maxWidth()));
		return cnv_table;
	}

	if(cnv_index_tumor_cn_change_ < 0 || cnv_index_tumor_clonality_ < 0)
	{
		cnv_table.addRow(RtfTableRow("Die ClinCNV-Datei enthält keine Tumor Clonality. Bitte mit einer aktuelleren Version von ClinCNV neu berechnen.",doc_.maxWidth()));
		return cnv_table;
	}

	//neccessary for filtering for target region
	//create set with target genes
	QString target_region = processing_system_data.target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region.left(target_region.size()-4) + "_genes.txt");
	target_genes = db_.genesToApproved(target_genes);

	int i_cnv_state = cnvs_filtered_.annotationIndexByName("state");

	for(int i=0; i<cnvs_filtered_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_filtered_[i];

		RtfTableRow temp_row;

		//coordinates
		QByteArrayList coords;
		coords << variant.chr().str() + ":" + QByteArray::number(variant.start() == 0 ? 1 : variant.start()) +"-" + QByteArray::number(variant.end());
		if(cnv_index_cnv_type_!=-1)
		{
			coords << "(" + variant.annotations().at(cnv_index_cnv_type_) +")";
		}
		temp_row.addCell(coords,2438);


		//AMP/DEL
		double tumor_copy_number_change = variant.annotations().at(cnv_index_tumor_cn_change_).toDouble();
		if(tumor_copy_number_change > 2.)
		{
			temp_row.addCell(962,"AMP");
		}
		else if(tumor_copy_number_change < 2.)
		{
			QByteArray del_text = "DEL";
			if(tumor_copy_number_change == 0.) del_text += " (hom)";
			else if(tumor_copy_number_change == 1.) del_text += " (het)";
			temp_row.addCell(962,del_text);
		}
		else
		{
			//check whether information about loss of heterocigosity is available
			if(i_cnv_state != -1 && variant.annotations().at(i_cnv_state) == "LOH")
			{
				temp_row.addCell(962,"LOH");
			}
			else
			{
				temp_row.addCell(962,"NA");
				temp_row.last().format().highlight(3);
			}
		}
		temp_row.last().format().setHorizontalAlignment("c");

		//copy numbers
		if(variant.annotations().at(cnv_index_tumor_cn_change_).toDouble() <= 6)
		{
			temp_row.addCell(400,variant.annotations().at(cnv_index_tumor_cn_change_));
		}
		else
		{
			temp_row.addCell(400,">6");
		}
		temp_row.last().format().setHorizontalAlignment("c");

		//tumor clonality
		temp_row.addCell(700,QByteArray::number(variant.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2),RtfParagraph().setHorizontalAlignment("c"));

		//gene names
		//only print genes which lie in target region
		if(!variant.annotations().at(cnv_index_cgi_genes_).isEmpty())
		{
			//cgi genes
			QByteArrayList cgi_genes = variant.annotations().at(cnv_index_cgi_genes_).split(',');

			QByteArrayList cgi_driver_statements = variant.annotations().at(cnv_index_cgi_driver_statement_).split(',');

			QByteArray genes_included = "";

			for(int j=0;j<cgi_genes.count();++j)
			{
				//skip if gene is not contained in target region
				if(!target_genes.contains(cgi_genes[j]))
				{
					continue;
				}

				//driver gene is printed bold
				if(cgi_driver_statements[j].contains("driver") || cgi_driver_statements[j].contains("known"))
				{
					genes_included.append(RtfText(cgi_genes[j]).setItalic(true).setBold(true).RtfCode());
				}
				else
				{
					genes_included.append(RtfText(cgi_genes[j]).setItalic(true).RtfCode());
				}

				if(j < cgi_genes.count()-1)
				{
					genes_included.append(", ");
				}
			}
			temp_row.addCell(5138,genes_included);
		}
		else //LOHs
		{
			if(i_cnv_state != -1 && variant.annotations().at(i_cnv_state) == "LOH")
			{
				GeneSet all_genes_loh = variant.genes().intersect(target_genes);

				RtfSourceCode tmp_entry = "";
				foreach(QByteArray gene,all_genes_loh)
				{
					tmp_entry.append(", " + RtfText(gene).setItalic(true).RtfCode());
				}
				tmp_entry = tmp_entry.mid(2); //remove initial ", "
				temp_row.addCell(5138,tmp_entry);
			}
			else
			{
				temp_row.addCell(5138,"");
			}
		}
		cnv_table.addRow(temp_row);
	}

	return cnv_table;
}

RtfTable SomaticReportHelper::createVirusTable()
{
	RtfTable table;
	table.addRow(RtfTableRow("Onkoviren",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(5));

	table.addRow(RtfTableRow({"Virus","Gen","Genom","Region","Abdeckung","Bewertung"},{963,964,1927,1927,1927,1929},RtfParagraph().setBold(true)));

	int validated_virus_count = 0;

	for(const auto& virus : viruses_)
	{
		if(virus.coverage_ <= 100) continue;
		if(virus.idendity_ < 90) continue;

		++validated_virus_count;

		RtfTableRow row;

		row.addCell(963,virus.virusName());

		row.addCell(964,virus.virusGene());
		row.addCell(1927,virus.chr_);

		QByteArray region = QByteArray::number(virus.start_) + "-" + QByteArray::number(virus.end_);
		row.addCell(1927,region);
		row.addCell(1927,QByteArray::number(virus.coverage_,'f',1));
		row.addCell(1929,"nachgewiesen*");

		table.addRow(row);
	}

	if(validated_virus_count == 0)
	{
		table.removeRow(1); //remove description
		table.addRow(RtfTableRow("Es wurde keines der untersuchten Virusgene nachgewiesen.",doc_.maxWidth()));
	}
	table.setUniqueBorder(1,"brdrhair",4);

	if(validated_virus_count > 0)
	{
		table.addRow(RtfTableRow("*Wir empfehlen eine Bestätigung des nachgewiesenen Onkovirus mit einer validierten Methode, beispielsweise am Institut für Medizinische Virologie und Epidemiologie der Viruskrankheiten.",doc_.maxWidth(),RtfParagraph().setSpaceBefore(50)));
		table.last().setBorders(0);
	}


	return table;
}


CGIDrugReportLine::CGIDrugReportLine()
{
}

const QString CGIDrugReportLine::proteinChange(QString aa_change)
{
	//CNVs
	if(aa_change.contains("del") && !aa_change.contains("amp")) return "DEL";
	if(aa_change.contains("amp") && !aa_change.contains("del")) return "AMP";

	//mutations

	//3 letters nomenclature
	bool known_format = true;
	if(aa_change.at(0).isDigit()) known_format = false;
	if(!aa_change.at(1).isDigit()) known_format = false;
	if(!aa_change.at(aa_change.count()-2).isDigit()) known_format = false;
	if(aa_change.at(aa_change.count()-1).isDigit()) known_format = false;

	if(known_format) //parse known nomenclatures
	{
		QString formatted_string = aa_change;

		QByteArray ref_aa = NGSHelper::expandAminoAcidAbbreviation(aa_change.at(0));
		QByteArray alt_aa = NGSHelper::expandAminoAcidAbbreviation(aa_change.at(aa_change.count()-1));

		formatted_string.remove(0,1);
		formatted_string.remove(formatted_string.count()-1,1);

		formatted_string.prepend(ref_aa);
		formatted_string.append(alt_aa);

		return formatted_string;
	}
	else if(aa_change.contains("fs*")) // parse frameshifts
	{
		QString formatted_string = aa_change.split("fs*")[0];

		QByteArray ref_aa = NGSHelper::expandAminoAcidAbbreviation(aa_change.at(0));
		formatted_string.remove(0,1);
		formatted_string.remove(formatted_string.count()-1,1);
		formatted_string.prepend(ref_aa);
		formatted_string.append("fs");
		return formatted_string;
	}
	else if(aa_change.contains("ins") && !aa_change.contains("del")) //parse pure(!) insertions
	{
		QString formatted_string = "";

		QString start_aa =aa_change.split("_")[0];
		start_aa.replace(0,1,NGSHelper::expandAminoAcidAbbreviation(start_aa[0]));
		QString end_aa = aa_change.split("ins")[0].split("_")[1];
		end_aa.replace(0,1,NGSHelper::expandAminoAcidAbbreviation(end_aa[0]));

		QString inserted_aa_old = aa_change.split("ins")[1];
		QString inserted_aa_new = "";
		for(int i=0;i<inserted_aa_old.count();++i)
		{
			inserted_aa_new.append(NGSHelper::expandAminoAcidAbbreviation(inserted_aa_old.at(i)));
		}
		formatted_string = start_aa + "_" + end_aa + "ins" + inserted_aa_new;
		return formatted_string;
	}

	//if parsing was not possible return raw aa_change string
	return aa_change;
}

CGIDrugTable::CGIDrugTable()
{
}

void CGIDrugTable::load(const QString &file_name)
{
	TSVFileStream file(file_name);

	int i_tumor_match = file.colIndex("TUMOR_MATCH",true);
	int i_alteration_match = file.colIndex("ALTERATION_MATCH",true);
	int i_sample_alteration = file.colIndex("SAMPLE_ALTERATION",true);
	int i_drug = file.colIndex("DRUG",true);
	int i_effect = file.colIndex("EFFECT",true);
	int i_evidence = file.colIndex("EVIDENCE",true);

	int i_source = file.colIndex("SOURCE",true);

	int i_tumor_entity = file.colIndex("TESTED_TUMOR",true);

	int i_variant_id = file.colIndex("SAMPLE",true);


	//group drugs into different groups

	QList< QList<QString> > all_drugs;

	while(!file.atEnd())
	{
		QList<QByteArray> line_tmp = file.readLine();
		QList<QString> line;
		for(int i=0; i<line_tmp.count(); ++i)
		{
			line.append(line_tmp.at(i));
		}

		all_drugs.append(line);
	}

	//parse text in "source", replace denominators
	for(int i=0; i<all_drugs.count(); ++i)
	{
		all_drugs[i][i_source] = all_drugs[i][i_source].replace(";",", ");
	}

	//parse text in evidence / source fields
	for(int i=0;i<all_drugs.count();++i)
	{
		//evidence
		if(all_drugs[i][i_evidence].contains("NCCN")) all_drugs[i][i_evidence].remove("NCCN");
		if(all_drugs[i][i_evidence].contains("FDA")) all_drugs[i][i_evidence].remove("FDA");
		all_drugs[i][i_evidence] = all_drugs[i][i_evidence].trimmed();

		//source
		if(all_drugs[i][i_source].contains("guidelines")) all_drugs[i][i_source].remove("guidelines");
	}

	//Group drugs by evidence level:
	//1: same tumor and evidence according guidlines
	//2: other tumor and evidence according guidelines of other tumor entity
	//3: same tumor and evidence according Late trials
	//4: other tumor and evidence according Late trials of other tumor entity
	//5: same tumor and evidence according Pre-clinical, Early Trials or Case report
	//6: other tumor and evidence according Pre-clinical, Early Trials or Case report
	foreach(QList<QString> line, all_drugs)
	{
		//skip all entries which differ in mutation
		if(line.at(i_alteration_match) != "complete") continue;

		CGIDrugReportLine row;

		row.id_ = line.at(i_variant_id);

		//Read in genes
		QStringList sample_alterations = line.at(i_sample_alteration).split(',');
		QStringList genes;
		foreach(QString alteration,sample_alterations)
		{
			if(alteration.contains("CNA")) genes.append(alteration.trimmed().split(':')[0]);
			if(alteration.contains("MUT")) genes.append(alteration.trimmed().split(' ')[0]);
		}
		genes.removeDuplicates();
		row.gene_ = genes[0];
		if(genes.count() > 1)
		{
			for(int i=1;i<genes.count();++i) row.gene_.append("," + genes[i]);
		}


		//parse protein changes / CNVs
		QStringList parsed_sample_alterations;
		foreach(QString alteration,sample_alterations)
		{
			QString gene;
			if(alteration.contains("CNA"))
			{
				gene = alteration.trimmed().split(':')[0];
				QString alteration_text = alteration.trimmed().split(':')[1];
				QString protein_change = CGIDrugReportLine::proteinChange(alteration_text);
				parsed_sample_alterations.append(gene+ ":" + protein_change);
			}
			else
			{
				gene = alteration.trimmed().split(' ')[0];
				QString alteration_text = alteration.split('(')[1];
				alteration_text.replace(")","");
				QString protein_change = CGIDrugReportLine::proteinChange(alteration_text);
				parsed_sample_alterations.append(gene + ':' + protein_change);
			}
		}
		row.alteration_type_ = parsed_sample_alterations[0];
		if(parsed_sample_alterations.count()>1)
		{
			for(int i=1;i<parsed_sample_alterations.count();++i) row.alteration_type_.append(","+parsed_sample_alterations[i]);
		}
		//Read in remaining line fields
		row.drug_ = line.at(i_drug).trimmed();
		row.drug_.replace(";",",");

		row.effect_ = line.at(i_effect);
		row.evidence_ = line.at(i_evidence);
		row.entity_ = line.at(i_tumor_entity);
		row.entity_.replace(", ",",");

		row.source_ = line.at(i_source);

		int evidence_level = 0;


		if(line.at(i_tumor_match) == "1" && line.at(i_evidence).contains("guidelines"))
		{
			evidence_level = 1;
		}
		if(line.at(i_tumor_match) == "0" && line.at(i_evidence).contains("guidelines"))
		{
			evidence_level = 2;
		}
		if(line.at(i_tumor_match) == "1" && line.at(i_evidence).contains("Late trials"))
		{
			evidence_level = 3;
		}
		if(line.at(i_tumor_match) == "0" && line.at(i_evidence).contains("Late trials"))
		{
			evidence_level = 4;
		}

		if(line.at(i_tumor_match) == "1" &&
		   (line.at(i_evidence).contains("Early trials") || line.at(i_evidence).contains("Pre-clinical") || line.at(i_evidence).contains("Case report")))
		{
			evidence_level = 5;
		}
		if(line.at(i_tumor_match) == "0" &&
		   (line.at(i_evidence).contains("Early trials") || line.at(i_evidence).contains("Pre-clinical") || line.at(i_evidence).contains("Case report")))
		{
			evidence_level = 6;
		}

		drug_list_.insert(evidence_level,row);
	}
}

void CGIDrugTable::removeDuplicateDrugs()
{
	foreach(CGIDrugReportLine line_1,drug_list_.values(1))
	{
		for(int i=2; i<=5; ++i) //remove starting at evidence level 2
		{
			foreach(CGIDrugReportLine line_2,drug_list_.values(i))
			{
				if(line_1 == line_2) drug_list_.remove(i,line_2);
			}
		}
	}
}

void CGIDrugTable::mergeDuplicates(int evid_level)
{
	//list with medicament names
	QMap<QString,int> drug_names_count;
	QStringList drug_names;

	foreach(CGIDrugReportLine line, drug_list_.values(evid_level))
	{
		drug_names.append(line.drug());
	}

	foreach(QString drug_name,drug_names)
	{
		++drug_names_count[drug_name];
	}

	QList<CGIDrugReportLine> erased_drugs;

	//erase duplicate drugs
	for(auto it = drug_list_.begin(); it != drug_list_.end(); ++it)
	{
		if(it.key() != evid_level) continue;

		if(drug_names_count[it.value().drug()] > 1)
		{
			erased_drugs.append(it.value());
			drug_list_.erase(it);

			--drug_names_count[it.value().drug()];
			it = drug_list_.begin();
		}
	}

	QList<CGIDrugReportLine> remaining_drugs;

	//fill in information from erased drugs if drugs can be merge (same tumor entity and gene)
	foreach(const CGIDrugReportLine& erased_drug,erased_drugs)
	{
		for(auto it=drug_list_.begin();it!=drug_list_.end();++it)
		{
			if(it.key() != evid_level) continue;

			if(erased_drug.entity() == it.value().entity() && erased_drug.drug() == it.value().drug() && erased_drug.gene() == it.value().gene() && erased_drug.effect() == it.value().effect() && erased_drug.source() != it.value().source())
			{
				it->setEvidence(it.value().evidence() + " / " + erased_drug.evidence());
				it->setSource(it.value().source() + " / " + erased_drug.source());
				break;
			}
			else
			{
				remaining_drugs << erased_drug;
				break;
			}
		}
	}

	foreach(const CGIDrugReportLine& drug, remaining_drugs)
	{
		drug_list_.insert(evid_level,drug);
	}

}



const QList<CGIDrugReportLine> CGIDrugTable::drugsSortedPerGeneName() const
{
	QList<CGIDrugReportLine> drugs;
	foreach(CGIDrugReportLine drug,drug_list_)
	{
		drugs.append(drug);
	}

	std::sort(drugs.begin(),drugs.end());

	return drugs;
}

SomaticReportHelper::SomaticReportHelper(QString snv_filename, const CnvList& filtered_cnvs, const FilterCascade& filters, const QString& target_region)
	: snv_filename_(snv_filename)
	, target_region_(target_region)
	, snv_germline_()
	, cnvs_filtered_(filtered_cnvs)
	, viruses_()
	, db_()
	, filters_(filters)
{
	//Apply somatic filters to SNV list
	VariantList temp_snv_variants;
	temp_snv_variants.load(snv_filename_);

	snv_variants_.copyMetaData(temp_snv_variants);
	QBitArray som_filters_pass = filters_.apply(temp_snv_variants).flags();

	for(int i=0;i<temp_snv_variants.count();++i)
	{
		if(!som_filters_pass[i]) continue;
		snv_variants_.append(temp_snv_variants[i]);
	}

	//Sort Variant list by gene
	snv_variants_.sortByAnnotation(snv_variants_.annotationIndexByName("gene",true,true));

	QString base_name = QFileInfo(snv_filename_).baseName();

	tumor_id_ = base_name.split('-')[0];
	normal_id_ = base_name.split('-')[1];

	cgi_drugs_path_ = QFileInfo(snv_filename_).absolutePath() + "/" + base_name + "_cgi_drug_prescription.tsv";

	//filename MANTIS output
	mantis_msi_path_ = QFileInfo(snv_filename_).absolutePath() + "/" + base_name + "_msi.tsv";
	try
	{
		TSVFileStream msi_file(mantis_msi_path_);
		//Use step wise difference (-> stored in the first line of MSI status file) for MSI status
		QByteArrayList data = msi_file.readLine();
		mantis_msi_swd_value_ = data[1].toDouble();
	}
	catch(...)
	{
		 mantis_msi_swd_value_ =  -1.;
	}

	//Path to virus
	QDir tmp_path = QDir(snv_filename_);
	tmp_path.cdUp();
	tmp_path.cdUp();
	tmp_path.cd("Sample_" + tumor_id_);


	try
	{
		QString path = tmp_path.absolutePath() + "/" + tumor_id_ + "_viral.tsv";
		TSVFileStream file(path);
		while(!file.atEnd())
		{
			QByteArrayList parts = file.readLine();
			if(parts.isEmpty()) continue;

			somatic_virus tmp;
			tmp.chr_ = parts[0];
			tmp.start_ = parts[1].toInt();
			tmp.end_ = parts[2].toInt();
			tmp.name_ = parts[file.colIndex("name",true)];
			tmp.reads_ = parts[file.colIndex("reads",true)].toInt();
			tmp.coverage_ = parts[file.colIndex("coverage",true)].toDouble();
			tmp.mismatches_ = parts[file.colIndex("mismatches",true)].toInt();
			tmp.idendity_ = parts[file.colIndex("identity\%",true)].toDouble();

			viruses_ << tmp;
		}
	}
	catch(...)
	{
		;//nothing to do here
	}

	//filename for germline SNV file
	germline_snv_filename_ = normal_id_ + ".GSvar";
	//find path for germline SNV file
	germline_snv_path_ = QDir(snv_filename_);
	germline_snv_path_.cdUp();
	germline_snv_path_.cdUp();
	germline_snv_path_.cd("Sample_" + normal_id_);

	try
	{
		snv_germline_.load(germline_snv_path_.absoluteFilePath(germline_snv_filename_));
	}
	catch(...) //if loading of snv_germline_ fails, use empty VariantList instead
	{
		QMessageBox::warning(NULL,"Control variants","Could not find file "+germline_snv_path_.absoluteFilePath(germline_snv_filename_)+" which contains control tissue variants.");
	}

	//extract obo terms for filtering
	OntologyTermCollection obo_terms("://Resources/so-xp_3_0_0.obo",true);
	QList<QByteArray> ids;
	ids << obo_terms.childIDs("SO:0001580",true); //coding variants
	ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
	foreach(const QByteArray& id, ids)
	{
		obo_terms_coding_splicing_.add(obo_terms.findByID(id));
	}

	//assign columns indices for SNV file
	snv_index_coding_splicing_ = snv_variants_.annotationIndexByName("coding_and_splicing",true,true);
	snv_index_cgi_driver_statement_ = snv_variants_.annotationIndexByName("CGI_driver_statement",true,true);
	snv_index_cgi_gene_role_ = snv_variants_.annotationIndexByName("CGI_gene_role",true,true);
	snv_index_cgi_transcript_ = snv_variants_.annotationIndexByName("CGI_transcript",true,true);
	snv_index_cgi_gene_ = snv_variants_.annotationIndexByName("CGI_gene",true,true);

	cnv_index_cn_change_ = cnvs_filtered_.annotationIndexByName("CN_change");
	cnv_index_cgi_gene_role_ = cnvs_filtered_.annotationIndexByName("CGI_gene_role");
	cnv_index_cnv_type_ = cnvs_filtered_.annotationIndexByName("cnv_type");
	cnv_index_cgi_genes_ = cnvs_filtered_.annotationIndexByName("CGI_genes");
	cnv_index_cgi_driver_statement_ = cnvs_filtered_.annotationIndexByName("CGI_driver_statement");
	cnv_index_tumor_clonality_ = cnvs_filtered_.annotationIndexByName("tumor_clonality");
	cnv_index_tumor_cn_change_ = cnvs_filtered_.annotationIndexByName("tumor_CN_change");


	//load qcml data
	QString qcml_file = base_name;
	qcml_file.append("_stats_som.qcML");
	QString qcml_file_absolute_path = QFileInfo(snv_filename).absolutePath().append("/").append(qcml_file);
	qcml_data_ = QCCollection::fromQCML(qcml_file_absolute_path);

	processing_system_data = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true);



	//load metadata from NGSD
	NGSD db;
	QString sample_id = db.sampleId(tumor_id_);
	QStringList tmp;
	QList<SampleDiseaseInfo> disease_info = db.getSampleDiseaseInfo(sample_id);

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
	histol_tumor_fraction_ = tmp.join(", ");

	tmp.clear();
	foreach(const SampleDiseaseInfo& entry,disease_info)
	{
		if(entry.type == "CGI cancer type") tmp.append(entry.disease_info);
	}
	if(tmp.count() == 1) cgi_cancer_type_ = tmp.first();
	else cgi_cancer_type_ = "";

	//Check whether GSVar CGI-annotation has the same cancer type as in NGSD
	if(cgi_cancer_type_ != cgiCancerTypeFromVariantList(snv_variants_))
	{
		THROW(DatabaseException,"CGI cancer type in .GSvar file is different from CGI cancer type stored in NGSD. Please correct either of them.");
	}

	//Add cgi cancer type to list of all CGI acronyms that occur in report
	cgi_acronyms_.append(cgi_cancer_type_.toLatin1());

	tmp.clear();
	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		if(entry.type == "HPO term id") tmp.append(entry.disease_info);
	}
	if(tmp.count() == 1) hpo_term_ = tmp.first();
	else hpo_term_ = "";


	try
	{
		mutation_burden_ = qcml_data_.value("QC:2000053",true).asString().split(' ')[1].remove('(').toDouble();
	}
	catch(...)
	{
		mutation_burden_ = std::numeric_limits<double>::quiet_NaN();
	}

	//load list with CGI acronyms, CGI explanation and TMB
	cgi_dictionary_ = cgi_info::load("://Resources/cancer_types.tsv");

	doc_.setMargins(1134,1134,1134,1134);

	doc_.addColor(188,230,138);
	doc_.addColor(255,0,0);
	doc_.addColor(255,255,0);
	doc_.addColor(161,161,161);
	doc_.addColor(242,242,242);
}

QByteArray SomaticReportHelper::cgiCancerTypeFromVariantList(const VariantList &variants)
{
	QStringList comments = variants.comments();
	foreach(QString comment,comments)
	{
		if(comment.startsWith("##CGI_CANCER_TYPE="))
		{
			QByteArray cancer_type = comment.mid(18).trimmed().toUtf8();
			if(!cancer_type.isEmpty()) return cancer_type;
			else return "n/a";
		}
	}
	return "n/a";
}

QList<QByteArray> SomaticReportHelper::parse_cgi_cancer_acronyms(QByteArray text)
{
	QList<QByteArray> cgi_acronyms = {};

	if(text.contains("known"))
	{
		text.replace("known in:","");
		text.replace(" ","");
		cgi_acronyms.append( text.split(';') );
		return cgi_acronyms;
	}
	return cgi_acronyms;
}

QMap<QByteArray, BedFile> SomaticReportHelper::gapStatistics(const BedFile& region_of_interest)
{
	BedFile roi_inter;
	QString processed_sample_id = db_.processedSampleId(tumor_id_);
	ProcessingSystemData system_data = db_.getProcessingSystemData(processed_sample_id, true);
	roi_inter.load(system_data.target_file);
	roi_inter.intersect(region_of_interest);
	roi_inter.merge();

	//Import BedFile with low coverage statistics
	QString low_cov_file = snv_filename_.left(snv_filename_.size()-6) + "_stat_lowcov.bed";
	BedFile low_cov;
	low_cov.load(low_cov_file);
	low_cov.intersect(region_of_interest);

	if(roi_inter.baseCount() != region_of_interest.baseCount())
	{
		QString message = "Gaps cannot be calculated because the selected target region is larger than the processing system target region:";
		BedFile roi_missing;

		roi_missing = region_of_interest;
		roi_missing.merge();
		roi_missing.subtract(roi_inter);
		for (int i=0; i<std::min(10, roi_missing.count()); ++i)
		{
			message += "\n" + roi_missing[i].toString(true);
		}

		QMessageBox::warning(NULL, "Invalid target region", message);
	}

	for(int i=0; i<low_cov.count(); ++i)
	{
		//note that line is reference&
		BedLine& line = low_cov[i];
		GeneSet genes = db_.genesOverlapping(line.chr(), line.start(), line.end(), 20); //extend by 20 to annotate splicing regions as well
		line.annotations().append(genes.join(", "));
	}

	//group by gene name
	QMap<QByteArray, BedFile> grouped;
	for (int i=0; i<low_cov.count(); ++i)
	{
		QList<QByteArray> genes = low_cov[i].annotations()[0].split(',');
		foreach(QByteArray gene, genes)
		{
			gene = gene.trimmed();

			//skip non-gene regions
			// - remains of VEGA database in old HaloPlex designs
			// - SNPs for sample identification
			if (gene=="") continue;
			grouped[gene].append(low_cov[i]);
		}
	}
	return grouped;
}

RtfTable SomaticReportHelper::createGapStatisticsTable(const QList<int>& col_widths)
{
	BedFile region_of_interest;
	region_of_interest.load(target_region_);

	GeneSet genes_in_target_region = GeneSet::createFromFile(target_region_.left(target_region_.size()-4)+"_genes.txt");

	QList<int> widths = col_widths;

	RtfTable table;

	table.addRow(RtfTableRow({"Zielregion:",QFileInfo(target_region_).fileName().toUtf8()},col_widths));

	if (!genes_in_target_region.isEmpty())
	{
		table.addRow(RtfTableRow({"Zielregion Gene (" + QByteArray::number(genes_in_target_region.count())+"):",genes_in_target_region.join(", ")},col_widths));
	}

	table.addRow(RtfTableRow({"Zielregion Region:",QByteArray::number(region_of_interest.count())},col_widths));

	table.addRow(RtfTableRow({"Zielregion Basen:",QByteArray::number(region_of_interest.baseCount())},col_widths));

	QString low_cov_file = snv_filename_;
	low_cov_file.replace(".GSvar", "_stat_lowcov.bed");
	BedFile low_cov;
	low_cov.load(low_cov_file);
	low_cov.intersect(region_of_interest);

	table.addRow(RtfTableRow({"Lücken Regionen:",QByteArray::number(low_cov.count())},col_widths));
	table.addRow(RtfTableRow({"Lücken Basen:",QByteArray::number(low_cov.baseCount()) + " (" +  QByteArray::number(100.0 * low_cov.baseCount()/region_of_interest.baseCount(), 'f', 2) + "%)"},col_widths));

	QMap<QByteArray, BedFile> grouped = gapStatistics(region_of_interest);

	//write gap statistics for each gene only if there are few genes
	if(grouped.keys().count() < 25)
	{
		widths.clear();
		widths << 900 << 1000 << 1000 << 6637;
		table.addRow(RtfTableRow({"Gen","Lücken","Chr","Koordinaten (GRCh37)"},widths,RtfParagraph().setBold(true).setSpaceBefore(50)));

		for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
		{
			const BedFile& gaps = it.value();
			QByteArray chr = gaps[0].chr().strNormalized(true);
			QByteArrayList coords;
			for (int i=0; i<gaps.count(); ++i)
			{
				coords << QByteArray::number(gaps[i].start()) + "\\_" + QByteArray::number(gaps[i].end());
			}
			table.addRow(RtfTableRow({RtfText(it.key()).setItalic(true).RtfCode(),QByteArray::number(gaps.baseCount()),chr,coords.join(", ")},widths,RtfParagraph()));
		}
	}
	return table;
}

RtfTable SomaticReportHelper::createQCTable(const QList<int> &widths)
{

	QDir directory = QFileInfo(snv_filename_).dir();
	directory.cdUp();
	QString qcml_file_tumor_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + tumor_id_ + "/" + tumor_id_ + "_stats_map.qcML";
	QString qcml_file_normal_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + normal_id_ + "/" + normal_id_ + "_stats_map.qcML";

	QCCollection qcml_data_tumor = QCCollection::fromQCML(qcml_file_tumor_stats_map_absolute_path);
	QCCollection qcml_data_normal = QCCollection::fromQCML(qcml_file_normal_stats_map_absolute_path);

	RtfTable table;
	table.addRow(RtfTableRow({"Analysepipeline: ", snv_variants_.getPipeline().toUtf8()},widths));

	table.addRow(RtfTableRow({"Auswertungssoftware:", QCoreApplication::applicationName().toUtf8()},widths));
	table.addRow(RtfTableRow({"Coverage Tumor 100x:",qcml_data_tumor.value("QC:2000030",true).toString().toUtf8()},widths));
	table.addRow(RtfTableRow({"Durchschnittliche Tiefe Tumor:",qcml_data_tumor.value("QC:2000025",true).toString().toUtf8() + "x"},widths));
	table.addRow(RtfTableRow({"Coverage Normal 100x:",qcml_data_normal.value("QC:2000030",true).toString().toUtf8()},widths));
	table.addRow(RtfTableRow({"Durchschnittliche Tiefe Normal:",qcml_data_normal.value("QC:2000025",true).toString().toUtf8() + "x"},widths));

	return table;
}


RtfTable SomaticReportHelper::createCgiDrugTable()
{
	RtfTable cgi_table;

	RtfParagraph header_format;
	header_format.setBold(true);
	header_format.setIndent(20,0,0);
	header_format.setFontSize(18);
	header_format.setHorizontalAlignment("c");
	cgi_table.addRow(RtfTableRow("Somatische Veränderungen mit CGI-Medikamenten-Annotation",doc_.maxWidth(),header_format).setBackgroundColor(5).setHeader());
	cgi_table.addRow(RtfTableRow({"Gen","Tumortyp","Medikament","Effekt","Evidenz","Quelle"},{1500,1200,2300,1000,1000,2638},header_format).setHeader());


	if(cgi_cancer_type_ == "CANCER" || cgi_cancer_type_ == "")
	{
		cgi_table.addRow(RtfTableRow("Cannot create drug table because CGI tumor type is set to " + cgi_cancer_type_.toUtf8() + ". Please restart CGI analysis with a properly selected cancer type",doc_.maxWidth(),RtfParagraph().highlight(3)));
		return cgi_table;
	}

	CGIDrugTable drugs;
	drugs.load(cgi_drugs_path_);

	drugs.removeDuplicateDrugs();

	drugs.mergeDuplicates(1);
	drugs.mergeDuplicates(2);
	drugs.mergeDuplicates(3);
	drugs.mergeDuplicates(4);
	drugs.mergeDuplicates(5);
	drugs.mergeDuplicates(6);

	//Make list of copy number altered genes which shall be kept in drug list
	GeneSet keep_cnv_genes;
	for(int i=0;i<cnvs_filtered_.count();++i)
	{
		keep_cnv_genes << cnvs_filtered_[i].genes();
	}

	//Get SNPs CGI IDs
	int i_snvs_variant_ids = snv_variants_.annotationIndexByName("CGI_id",true,false);
	if(i_snvs_variant_ids == -1)
	{
		cgi_table.addRow(RtfTableRow("Cannot create drug table because CGI annotation is too old (there is no \"CGI_id\" column in GSVar file). Please restart CGI analysis with the latest megSAP version.",doc_.maxWidth(),RtfParagraph().highlight(3)));
		return cgi_table;
	}
	QList<QString> variant_ids;
	for(int i=0;i<snv_variants_.count();++i)
	{
		variant_ids.append(snv_variants_[i].annotations().at(i_snvs_variant_ids));
	}

	QList<CGIDrugReportLine> drugs_sorted = drugs.drugsSortedPerGeneName();

	foreach(CGIDrugReportLine drug,drugs_sorted)
	{
		bool is_cnv = false;
		if(drug.alterationType().contains("AMP") || drug.alterationType().contains("DEL")) is_cnv = true;

		//Only include drugs that refer to an alteration in SNV/CNV report
		bool alt_found = false;
		if(is_cnv)
		{
			QStringList genes = drug.gene().split(',');
			foreach(QString gene,genes)
			{
				if(keep_cnv_genes.contains(gene.toLatin1()))
				{
					alt_found = true;
					break;
				}
			}
		}
		else
		{
			foreach(QString snv_id,variant_ids)
			{
				if(snv_id == drug.id())
				{
					alt_found = true;
					break;
				}
			}
		}
		if(!alt_found) continue;

		//Alteration type / tumor type / drug / effect / evidence /source
		QList<QByteArray> result_line;

		for(const auto& entry : drug.asStringList())
		{
			result_line.append(entry.toUtf8());
		}

		//remove first column because gene name is included in alteration type already
		result_line.removeAt(0);

		//gene names should be printed italic
		result_line[0].prepend("\\i ");
		result_line[0].replace(":","\\i0 :");
		result_line[0].replace(",",",\\i  ");
		result_line[0].append("\\i0 ");

		//tumor type: add space in case of multiple entities
		result_line[1].replace(",",", ");

		cgi_table.addRow(RtfTableRow(result_line,{1500,1200,2300,1000,1000,2638},RtfParagraph().setIndent(20,0,0).setFontSize(16)));

		//Update cancer acronym list
		QString tmp_cancer_acronyms = drug.entity();
		QByteArrayList cancer_acronyms = tmp_cancer_acronyms.replace(" ","").toLatin1().split(',');
		foreach(QByteArray acronym,cancer_acronyms)
		{
			cgi_acronyms_.append(acronym);
		}
	}
	if(cgi_table.isEmpty())
	{
		cgi_table.addRow(RtfTableRow("Es wurden keine Medikamente durch CGI annotiert.",doc_.maxWidth(),RtfParagraph()));
	}

	return cgi_table;
}

void SomaticReportHelper::germlineSnvForQbic()
{
	QString path_target_folder = QFileInfo(snv_filename_).absolutePath() + "/" + "QBIC_files/";

	if(!QDir(path_target_folder).exists())
	{
		QDir().mkdir(path_target_folder);
	}

	//currently no germline SNVs are uploaded, only created header
	QSharedPointer<QFile> germline_snvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_germline_snv.tsv");

	QTextStream stream(germline_snvs_qbic.data());

	stream << "chr" << "\t" << "start" << "\t" << "ref" << "\t" << "alt" << "\t" << "genotype" << "\t";
	stream << "gene" << "\t" << "base_change" << "\t" << "aa_change" << "\t" << "transcript" << "\t";
	stream << "functional_class" << "\t" << "effect";
	stream << endl;

	germline_snvs_qbic->close();
}

void SomaticReportHelper::somaticSnvForQbic()
{
	QString path_target_folder = QFileInfo(snv_filename_).absolutePath() + "/" + "QBIC_files/";

	if(!QDir(path_target_folder).exists())
	{
		QDir().mkdir(path_target_folder);
	}
	VariantList variants = gsvarToVcf(snv_variants_,snv_filename_.replace(".GSvar","_var_annotated.vcf.gz"));

	QSharedPointer<QFile> somatic_snvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_somatic_snv.tsv");

	QTextStream stream(somatic_snvs_qbic.data());

	//Write header
	stream << "chr" <<"\t" << "start" << "\t" << "ref" << "\t" << "alt" << "\t";
	stream <<"allele_frequency_tumor" << "\t" << "coverage" << "\t";
	stream << "gene" << "\t" << "base_change" << "\t" << "aa_change" << "\t";
	stream << "transcript" << "\t" << "functional_class" << "\t" << "effect" << endl;


	int i_tumor_af = variants.annotationIndexByName("tumor_af",true,true);
	int i_tumor_depth = variants.annotationIndexByName("tumor_dp",true,true);

	for(int i=0; i<variants.count(); ++i)
	{
		const Variant& variant = variants[i];

		stream << variant.chr().str() << "\t";
		stream << variant.start() << "\t";
		stream << variant.ref() << "\t";
		stream << variant.obs()<< "\t";
		stream << variant.annotations().at(i_tumor_af) << "\t";
		stream << variant.annotations().at(i_tumor_depth) << "\t";

		//determine transcript, if available take CGI transcript, first coding/splicing if not available, first otherwise
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
		QByteArray effect;
		bool is_driver = false;

		if(variant.annotations().at(snv_index_cgi_driver_statement_).contains("known") || variant.annotations().at(snv_index_cgi_driver_statement_).contains("driver"))
		{
			is_driver = true;
		}
		if(is_driver && variant.annotations().at(snv_index_cgi_gene_role_) == "Act")
		{
			effect = "activating";
		}
		else if(is_driver && variant.annotations().at(snv_index_cgi_gene_role_) == "LoF")
		{
			effect = "inactivating";
		}
		else if(is_driver && variant.annotations().at(snv_index_cgi_gene_role_) == "ambiguous")
		{
			effect = "ambiguous";
		}
		else if(!is_driver)
		{
			effect = "NA";
		}

		bool is_coding_splicing = transcript.typeMatchesTerms(obo_terms_coding_splicing_);
		bool cgi_transcript_exists = true;
		if(transcript.id != variant.annotations().at(snv_index_cgi_transcript_)) cgi_transcript_exists = true;

		//if CGI transcript does not agree with our prediction that the SNV is coding/splicing, set CGI effect to NA
		if(!is_coding_splicing || !cgi_transcript_exists)
		{
			effect = "NA";
		}

		stream << effect;

		stream << endl;
	}
	somatic_snvs_qbic->close();
}

void SomaticReportHelper::germlineCnvForQbic()
{
	QString path_target_folder = QFileInfo(snv_filename_).absolutePath() + "/" + "QBIC_files/";

	if(!QDir(path_target_folder).exists())
	{
		QDir().mkdir(path_target_folder);
	}

	QSharedPointer<QFile> germline_cnvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_germline_cnv.tsv");

	QTextStream stream(germline_cnvs_qbic.data());

	stream << "size" << "\t" << "type" << "\t" << "copy_number" << "\t" << "gene" << "\t" << "exons" << "\t" << "transcript" << "\t";
	stream << "chr" << "\t" << "start" << "\t" << "end" << "\t" << "effect";
	stream << endl;

	germline_cnvs_qbic->close();
}


void SomaticReportHelper::somaticCnvForQbic()
{
	QString path_target_folder = QFileInfo(snv_filename_).absolutePath() + "/" + "QBIC_files/";

	if(!QDir(path_target_folder).exists())
	{
		QDir().mkdir(path_target_folder);
	}

	QSharedPointer<QFile> somatic_cnvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_somatic_cnv.tsv");

	QTextStream stream(somatic_cnvs_qbic.data());

	stream << "size" << "\t" << "type" << "\t" << "copy_number" << "\t" << "gene" << "\t" << "exons" << "\t";
	stream << "transcript" << "\t" << "chr" << "\t" << "start" << "\t" << "end" << "\t" << "effect" << endl;

	//Abort if CGI driver statement is invalid
	if(cnv_index_cgi_driver_statement_ < 0 || cnv_index_cgi_genes_ < 0 || cnv_index_cgi_gene_role_ < 0)
	{
		somatic_cnvs_qbic->close();
		return;
	}

	QString target_region_processing_system = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true).target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region_processing_system.left(target_region_processing_system.size()-4) + "_genes.txt");
	NGSD db;
	target_genes = db.genesToApproved(target_genes);

	for(int i=0; i < cnvs_filtered_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_filtered_[i];

		GeneSet genes_in_report = target_genes.intersect(GeneSet::createFromText(variant.annotations().at(cnv_index_cgi_genes_),','));

		if(cnv_index_cnv_type_ < 0)
		{
			stream << "";
		}
		else
		{
			stream << variant.annotations().at(cnv_index_cnv_type_);
		}
		stream << "\t";

		//take copy number twice (-> total number of copys instead normalized to diploid chormosomes)
		double copy_number = variant.annotations().at(cnv_index_cn_change_).toDouble();

		if(copy_number > 2)
		{
			stream << "amp";
		}
		else
		{
			stream << "del";
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

		//effect
		QByteArrayList effects = variant.annotations().at(cnv_index_cgi_gene_role_).split(',');
		QByteArrayList genes = variant.annotations().at(cnv_index_cgi_genes_).split(',');
		QByteArrayList driver_statements = variant.annotations().at(cnv_index_cgi_driver_statement_).split(',');
		QList<bool> is_driver;
		for(int j=0;j<effects.count();++j)
		{
			if(driver_statements[j].contains("known") || driver_statements[j].contains("driver"))
			{
				is_driver.append(true);
			}
			else
			{
				is_driver.append(false);
			}
		}

		bool is_any_effect_reported = false;
		for(int j=0;j<effects.count();++j)
		{
			if(effects[j].isEmpty()) continue;

			QByteArray effect = "";

			if(is_driver[j] && effects[j] == "Act")
			{
				effect = "activating";
			}
			else if(is_driver[j] && effects[j] == "LoF")
			{
				effect = "inactivating";
			}
			else if(is_driver[j] && effects[j] == "ambiguous")
			{
				effect = "ambiguous";
			}
			else if(!is_driver[j]) //do not report genes which have none of these effects
			{
				continue;
			}

			is_any_effect_reported = true;
			stream << genes[j] << ":" << effect;

			if(j<effects.count()-1 && is_driver[j+1] ) stream << ";";
		}
		if(!is_any_effect_reported) stream << "NA";

		stream << endl;
	}
	somatic_cnvs_qbic->close();
}

void SomaticReportHelper::somaticSvForQbic()
{
	QString path_target_folder = QFileInfo(snv_filename_).absolutePath() + "/" + "QBIC_files/";

	if(!QDir(path_target_folder).exists())
	{
		QDir().mkdir(path_target_folder);
	}

	QSharedPointer<QFile> somatic_sv_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_somatic_sv.tsv");

	QTextStream stream(somatic_sv_qbic.data());

	stream << "type" << "\t" << "gene" << "\t" << "effect" << "\t" << "left_bp" << "\t" << "right_bp" << endl;

	somatic_sv_qbic->close();

}

void SomaticReportHelper::metaDataForQbic()
{
	QString path_target_folder = QFileInfo(snv_filename_).absolutePath() + "/" + "QBIC_files/";

	if(!QDir(path_target_folder).exists())
	{
		QDir().mkdir(path_target_folder);
	}

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

	stream << db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true).genome;
	stream << endl;

	meta_data_qbic->close();

}

VariantList SomaticReportHelper::gsvarToVcf(const VariantList& gsvar_list, const QString& orig_name)
{
	BedFile roi;
	for(int i=0;i<gsvar_list.count();++i)
	{
		roi.append(BedLine(gsvar_list[i].chr(), gsvar_list[i].start()-15, gsvar_list[i].end()+15));
	}
	roi.merge();

	VariantList output;
	output.copyMetaData(gsvar_list);

	if(!QFile::exists(orig_name))
	{
		THROW(FileParseException,"Could not find vcf file " + orig_name + " to convert GSvar to VCF format.");
		return output;
	}

	VariantList orig_vcf;
	orig_vcf.load(orig_name,VCF_GZ, &roi);

	ChromosomalIndex<VariantList> orig_idx(orig_vcf);

	for(int i=0;i<gsvar_list.count();++i)
	{
		const Variant& v = gsvar_list[i];

		int hit_count = 0;

		QVector<int> matches = orig_idx.matchingIndices(v.chr(), v.start()-10, v.end()+10);

		QList<QByteArray> annotations = v.annotations();

		foreach(int index, matches)
		{
			Variant v2 = orig_vcf[index];

			if (v.isSNV()) //SNV
			{
				if (v.start()==v2.start() && v.obs()==v2.obs())
				{
					v2.setAnnotations(annotations);
					output.append(v2);
					++hit_count;
				}
			}
			else if (v.ref()=="-") //insertion
			{
				if (v.start()==v2.start() && v2.ref().count()==1 && v2.obs().mid(1)==v.obs())
				{
					v2.setAnnotations(annotations);
					output.append(v2);
					++hit_count;
				}
			}
			else if (v.obs()=="-") //deletion
			{
				if (v.start()-1==v2.start() && v2.obs().count()==1 && v2.ref().mid(1)==v.ref())
				{
					v2.setAnnotations(annotations);
					output.append(v2);
					++hit_count;
				}
			}
			else //complex
			{
				if (v.start()==v2.start() && v2.obs()==v.obs() && v2.ref()==v.ref())
				{
					v2.setAnnotations(annotations);
					output.append(v2);
					++hit_count;
				}
			}
		}
		if (hit_count!=1)
		{
			THROW(ProgrammingException, "Found " + QString::number(hit_count) + " matching variants for " + v.toString() + " in VCF file. Exactly one expected!");
		}
	}
	return output;
}

VariantTranscript SomaticReportHelper::selectSomaticTranscript(const Variant& variant)
{
	QList<VariantTranscript> transcripts = variant.transcriptAnnotations(snv_index_coding_splicing_);

	//first coding/splicing transcript which has the same Ensembl ID as in CGI annotation
	QString cgi_transcript = variant.annotations().at(snv_index_cgi_transcript_); //Ensembl-ID of CGI transcript
	cgi_transcript = cgi_transcript.remove(',');
	cgi_transcript = cgi_transcript.trimmed();
	foreach(const VariantTranscript& trans, transcripts)
	{
		if(trans.id != cgi_transcript) continue;

		if(trans.typeMatchesTerms(obo_terms_coding_splicing_))
		{
			return trans;
		}
	}

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

QByteArray SomaticReportHelper::getCnvType(const CopyNumberVariant& cnv)
{
	bool success = false;
	double tumor_cn = cnv.annotations().at(cnv_index_tumor_cn_change_).toDouble(&success);
	QByteArray type = "";
	if(success)
	{
		if(tumor_cn > 2)
		{
			if(tumor_cn <= 6) type = "AMP (" + QByteArray::number((int)tumor_cn) + " copies)";
			else type = "AMP (> 6 copies)";
		}
		else if(tumor_cn < 2)
		{
			type = "DEL";
			if(tumor_cn == 0 ) type.append(" (hom)");
			else if(tumor_cn == 1) type.append(" (het)");
		}
		else if(tumor_cn == 2) type = "LOH";
		else type = "NA";
	}

	return type;
}

double SomaticReportHelper::getCnvMaxTumorClonality(const CnvList& cnvs)
{
	int i_cnv_tum_clonality = cnvs.annotationIndexByName("tumor_clonality");
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


void SomaticReportHelper::writeRtf(const QByteArray& out_file)
{
	/**********************************
	 * MUTATION BURDEN AND MSI STATUS *
	 **********************************/
	RtfTable general_info_table;

	general_info_table.addRow(RtfTableRow("Allgemeine genetische Charakteristika",doc_.maxWidth(),RtfParagraph().setBold(true)).setBackgroundColor(5).setBorders(1,"brdrhair",4));

	general_info_table.addRow(RtfTableRow({"Mutationslast:", QByteArray::number(mutation_burden_) + " Var/Mbp"},{2500,7137}).setBorders(1,"brdrhair",4));

	general_info_table.last()[0].setBorder(1,1,1,0,"brdrhair");
	general_info_table.last().last().setBorder(1,1,1,0,"brdrhair");

	//Parse reference data from publication
	QList<tmb_info> hpo_tmbs = tmb_info::load("://Resources/hpoterms_tmb.tsv");
	int entries_count = 0;
	foreach(tmb_info data,hpo_tmbs)
	{
		if(data.hpoterm == hpo_term_) ++entries_count;
	}

	RtfTableRow ref_values;
	ref_values.addCell(2500,"Vergleichswerte:",RtfParagraph().setFontSize(14));

	if(entries_count == 0)
	{
		if(!hpo_term_.isEmpty()) ref_values.addCell(7137,"Es sind keine Vergleichsdaten zu dieser Tumorentität erfasst. (PMID: 28420421)",RtfParagraph().setFontSize(14));
		else ref_values.addCell(7137,"Es wurde kein (eindeutiger) HPO-Term in NGSD hinterlegt.",RtfParagraph().setFontSize(14));
	}
	else
	{
		QByteArrayList tmp;
		foreach(tmb_info data,hpo_tmbs)
		{
			if(data.hpoterm == hpo_term_)
			{
				tmp << "Median: " + QByteArray::number(data.tmb_median) + " Var/Mbp, Maximum: " + QByteArray::number(data.tmb_max) + " Var/Mbp, Probenanzahl: " + QByteArray::number(data.cohort_count) + " (PMID: 28420421) "
										+ RtfText(data.tumor_entity).highlight(3).setFontSize(14).RtfCode();
			}
		}
		ref_values.addCell(7137,tmp.join("\n\\line\n"),RtfParagraph().setFontSize(14));

		ref_values.setBorders(1,"brdrhair",4);
		ref_values[0].setBorder(10,0,10,10,"brdrhair");
		ref_values.last().setBorder(10,0,10,10,"brdrhair");
	}
	general_info_table.addRow(ref_values);


	//MSI statis, values larger than 0.4 are considered unstable
	general_info_table.addRow(RtfTableRow({"MSI-Status:",QByteArray::number(mantis_msi_swd_value_,'f',3)+" (" + (mantis_msi_swd_value_ < 0.4 ? "stabil" : "instabil")+")"},{2500,7137}).setBorders(1,"brdrhair",4));

	double tumor_molecular_proportion;

	try
	{
		 tumor_molecular_proportion = qcml_data_.value("QC:2000054",true).toString().toDouble();
	}
	catch(...)
	{
		tumor_molecular_proportion = std::numeric_limits<double>::quiet_NaN();
	}

	double tumor_cnv_proportion = getCnvMaxTumorClonality(cnvs_filtered_) * 100.;

	if(tumor_molecular_proportion > 100.)	tumor_molecular_proportion = 100.;

	general_info_table.addRow(RtfTableRow({"Tumoranteil (hist. / bioinf.):", histol_tumor_fraction_.toUtf8() + "\%" + " / " +QByteArray::number(tumor_molecular_proportion,'f',1) + "\%" + " / " + QByteArray::number(tumor_cnv_proportion,'f',1) + "\%"},{2500,7137}).setBorders(1,"brdrhair",4));

	//Calc percentage of CNV altered genome
	double cnv_altered_percentage = cnvs_filtered_.totalCnvSize() / 3101788170. * 100;
	if(cnv_altered_percentage >= 0.01)
	{
		general_info_table.addRow(RtfTableRow({"CNV-Last:", QByteArray::number(cnv_altered_percentage,'f',1) + "\%"},{2500,7137}).setBorders(1,"brdrhair",4));
	}
	else
	{
		general_info_table.addRow(RtfTableRow({"CNV-Last:", "Keine CNVs nachgewiesen"},{2500,7137},RtfParagraph().highlight(3)).setBorders(1,"brdrhair",4));
	}
	doc_.addPart(general_info_table.RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());
	RtfParagraph text_helper("In der untersuchten Tumorprobe konnte eine Vielzahl struktureller ");
	text_helper.setHorizontalAlignment("j");
	text_helper.append(RtfText("(bei Fusionen oder bei vielen Brüchen innerhalb eines Chromosoms)").highlight(3).RtfCode());
	text_helper.append(" und numerischer Chromosomenaberrationen in Form von ");
	text_helper.append(RtfText("Amplifikationen / und von Deletionen").highlight(3).RtfCode());
	text_helper.append(" größerer chromosomaler Bereiche und ganzer Chromosomen nachgewiesen werden. In der nachfolgenden Liste befinden sich alle Kopienzahlveränderungen, die durch die Datenbank ");
	text_helper.append(RtfText("Cancer Genome Interpreter").setItalic(true).RtfCode());
	text_helper.append(" (CGI) als funktionell relevant eingestuft wurden. ");
	text_helper.append(RtfText("Es gibt Hinweise auf eine chromosomale Instabilität.").highlight(3).RtfCode());
	doc_.addPart(text_helper.setIndent(0,0,0).RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());

	text_helper.setContent("Eine Auswertung der somatischen Kopienzahlveränderungen war aufgrund ");
	text_helper.append(RtfText("des niedrigen Tumorgehalts / der Qualität der DNA / der Heterogenität des Tumors").highlight(3).RtfCode());
	text_helper.append(" nur eingeschränkt möglich. Es zeigte sich ein komplexes Bild, das am ehesten mit einer Polysomie ");
	text_helper.append(RtfText("und").highlight(3).RtfCode());
	text_helper.append(" mit einer Monosomie mehrerer Chromosomen vereinbar ist. Folgende Gene sind möglicherweise von chromosomalen Aberrationen betroffen.");
	doc_.addPart(text_helper.setIndent(0,0,0).RtfCode());


	/********************************
	 * RELEVANT SOMATIC ALTERATIONS *
	 ********************************/
	doc_.newPage();
	doc_.addPart(RtfParagraph("Potentiell relevante somatische Veränderungen:").setBold(true).setIndent(0,0,0).RtfCode());

	VariantList snvs_to_be_printed;
	snvs_to_be_printed.copyMetaData(snv_variants_);

	QBitArray pass(snv_variants_.count(),false);

	if(target_region_.isEmpty()) //Include drivers in first part only (HSA report)
	{
		for(int i=0;i<snv_variants_.count();++i)
		{
			QByteArray statement = snv_variants_[i].annotations().at(snv_index_cgi_driver_statement_);
			if(statement.contains("driver") || statement.contains("known"))
			{
				pass[i] = true;
			}
		}
	}
	else //Remove SNVs outside target region (EBM report)
	{
		BedFile target_bed;
		target_bed.load(target_region_);
		FilterResult filter_result(snv_variants_.count());
		FilterRegions::apply(snv_variants_,target_bed,filter_result);
		pass = filter_result.flags();
	}

	for(int i=0;i<pass.count();++i)
	{
		if(pass[i]) snvs_to_be_printed.append(snv_variants_[i]);
	}

	QString target_genes_file = target_region_.left(target_region_.size()-4) + "_genes.txt";
	GeneSet target_genes;
	if(!target_region_.isEmpty() && QFile::exists(target_genes_file))
	{
		 target_genes = GeneSet::createFromFile(target_genes_file);
		 target_genes = db_.genesToApproved(target_genes,true);
	}

	doc_.addPart(somaticAlterationTable(snvs_to_be_printed,cnvs_filtered_,true,target_genes).setUniqueBorder(1,"brdrhair",4).RtfCode());


	RtfSourceCode snv_expl  = RtfText("Fraction:").setBold(true).setFontSize(14).RtfCode() + " Frequenz der Variante bzw. der Probenanteil der entsprechenden Kopienzahlvariante. ";
	snv_expl += RtfText("Description:").setFontSize(14).setBold(true).RtfCode() + " Klassifikation der Varianten durch CGI, Genname und ggf. Bewertung der Genfunktion als Onkogen bzw. Tumorsuppressorgen (TSG). ";
	snv_expl += "Erweiterte Legende und Abkürzungen siehe Anlage 1.";
	doc_.addPart(RtfParagraph(snv_expl).setFontSize(14).setIndent(0,0,0).setHorizontalAlignment("j").RtfCode());

	doc_.addPart(RtfParagraph(" ").RtfCode());


	/***********
	 * FUSIONS *
	 ***********/
	RtfTable fusions_table;

	fusions_table.addRow(RtfTableRow("Fusionen",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(5));

	//Check whether fusions file contains data
	QString fusions_file = snv_filename_.left(snv_filename_.size()-6) + "_var_structural.tsv";
	if(QFile::exists(fusions_file))
	{
		TSVFileStream fusions(fusions_file);

		if(fusions.atEnd())
		{
			fusions_table.addRow(RtfTableRow("Es wurden keine Fusionen gefunden.",doc_.maxWidth()));
		}
		else
		{
			fusions_table.addRow(RtfTableRow("Fusionen gefunden. Bitte "+fusions_file.toUtf8()+" prüfen.",doc_.maxWidth(),RtfParagraph().highlight(3)));
		}
	}
	fusions_table.setUniqueBorder(1,"brdrhair",4);
	doc_.addPart(fusions_table.RtfCode());

	/***************
	 * ONCOVIRUSES *
	 ***************/
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.addPart(createVirusTable().RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.newPage();

	/*********************
	 * ADDITIONAL REPORT *
	 *********************/
	doc_.addPart(RtfParagraph("Alle nachgewiesenen somatischen Veränderungen:").setBold(true).setSpaceAfter(45).setFontSize(18).RtfCode());

	doc_.addPart(somaticAlterationTable(snv_variants_,cnvs_filtered_,false).setUniqueBorder(1,"brdrhair",4).RtfCode());

	doc_.addPart(RtfParagraph(" ").RtfCode());

	//Make rtf report, filter genes for processing system target region
	doc_.addPart(createCnvTable().setUniqueBorder(1,"brdrhair",4).RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());
	/**************
	 * DRUG TABLE *
	 **************/
	doc_.addPart(createCgiDrugTable().setUniqueBorder(1,"brdrhair",4).RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.newPage();

	/***********************
	 * GENERAL INFORMATION *
	 **********************/
	doc_.addPart(RtfParagraph("Allgemeine Informationen").setSpaceAfter(45).setFontSize(18).setBold(true).RtfCode());

	RtfTable metadata;

	metadata.addRow(RtfTableRow({"Datum:",QDate::currentDate().toString("dd.MM.yyyy").toUtf8()},{2500,7137}));
	metadata.addRow(RtfTableRow({"Proben-ID (Tumor):",tumor_id_.toUtf8()},{2500,7137}));
	metadata.addRow(RtfTableRow({"Proben-ID (Keimbahn):",normal_id_.toUtf8()},{2500,7137}));
	metadata.addRow(RtfTableRow({"CGI-Tumortyp:",cgi_cancer_type_.toUtf8()},{2500,7137}));

	GeneSet gene_set = GeneSet::createFromFile(processing_system_data.target_file.left(processing_system_data.target_file.size()-4) + "_genes.txt");
	metadata.addRow(RtfTableRow({"Prozessierungssystem:",processing_system_data.name.toUtf8() + " (" + QByteArray::number(gene_set.count()) + ")"},{2500,7137}));
	doc_.addPart(metadata.RtfCode());

	/***************************************
	 * QUALITY PARAMETERS / GAP STATISTICS *
	 ***************************************/
	//write QC params here in case of HSA report

	doc_.addPart(RtfParagraph("").RtfCode());
	doc_.addPart(RtfParagraph("Qualitätsparameter").setSpaceAfter(45).setSpaceBefore(45).setFontSize(18).setBold(true).RtfCode());
	doc_.addPart(createQCTable( { 3000, 6637 }).RtfCode());

	if(!target_region_.isEmpty()) //For EBM report only: gap statistics
	{
		doc_.addPart(RtfParagraph("").RtfCode());
		doc_.addPart(RtfParagraph("Lückenstatistik:").setBold(true).setSpaceAfter(45).setSpaceBefore(45).setFontSize(18).RtfCode());
		doc_.addPart(createGapStatisticsTable({2200,7437}).RtfCode());
	}

	/*********************
	 * CGI ACRONYMS LIST *
	 *********************/
	doc_.addPart(RtfParagraph("").RtfCode());

	RtfTable cgi_acronyms_table;
	cgi_acronyms_table.addRow(RtfTableRow("Abkürzungsverzeichnis",doc_.maxWidth(),RtfParagraph().setBold(true).setSpaceAfter(45).setSpaceBefore(45).setFontSize(18)));

	QByteArrayList cgi_acronym_explanation;
	foreach(cgi_info data,cgi_dictionary_)
	{
		if(cgi_acronyms_.contains(data.acronym)) cgi_acronym_explanation << data.acronym + ": " + data.def_german;
	}
	cgi_acronyms_table.addRow(RtfTableRow(cgi_acronym_explanation.join(", "),doc_.maxWidth()));

	doc_.addPart(cgi_acronyms_table.RtfCode());

	doc_.save(out_file);
}
