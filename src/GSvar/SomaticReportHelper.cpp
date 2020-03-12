#include "SomaticReportHelper.h"
#include "BasicStatistics.h"
#include "OntologyTermCollection.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "NGSHelper.h"
#include "FilterWidget.h"
#include "ReportWorker.h"
#include "SomaticReportSettings.h"
#include "NGSD.h"

#include <cmath>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMainWindow>
#include <QSqlError>
#include <QMap>
#include <QPair>
#include <QCollator>

RtfTable SomaticReportHelper::somaticAlterationTable(const VariantList& snvs, const CnvList& cnvs, bool include_cnvs, const GeneSet& target_genes, bool sort_by_gene)
{
	RtfTable table;

	//Make list of somatic variants in control tissue, chosen according report settings
	VariantList som_var_in_normal = SomaticReportSettings::filterGermlineVariants(snv_germline_, settings_);

	QByteArray heading_text = "Punktmutationen (SNVs), kleine Insertionen/Deletionen (INDELs)";
	if(include_cnvs) heading_text +=  " und Kopienzahlvarianten (CNVs)";
	table.addRow(RtfTableRow(heading_text,doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(5).setHeader());
	table.addRow(RtfTableRow({"Gen","Veränderung","Typ","Anteil","Beschreibung"},{1000,2900,1700,900,3138},RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader());

	if(snvs.count() == 0 && cnvs.isEmpty() && som_var_in_normal.count() == 0)
	{
		table.removeRow(1); //remove description header
		table.addRow(RtfTableRow("Es wurden keine potentiell relevanten somatischen Veränderungen nachgewiesen.",doc_.maxWidth()));
		return table;
	}

	int i_cnv_tum_clonality = cnvs.annotationIndexByName("tumor_clonality", false);
	if(i_cnv_tum_clonality < 0 && cnvs.count() > 0)
	{
		table.addRow(RtfTableRow("No column \"tumor_clonality\" in ClinCNV file. Please recalculate it using a more recent ClinCNV version.", doc_.maxWidth(),RtfParagraph().highlight(3)));
		return table;
	}

	GeneSet genes_in_first_part; //cn altered genes in first part of table

	//Add germline snvs to temporary set
	int i_germl_gene = som_var_in_normal.annotationIndexByName("gene");
	int i_germl_co_sp = som_var_in_normal.annotationIndexByName("coding_and_splicing");
	int i_germl_freq_in_tum = som_var_in_normal.annotationIndexByName("freq_in_tum");
	int i_germl_hom_het = som_var_in_normal.annotationIndexByName(settings_.normal_ps);
	int i_germl_class = som_var_in_normal.annotationIndexByName("classification");


	QList<QPair<Variant, RtfTableRow>> temp_rows; //combined list of germline/somatic snvs

	for(int i=0; i< som_var_in_normal.count(); ++i) //insert next to gene that is already included
	{
		const Variant& var = som_var_in_normal[i];

		RtfTableRow temp;
		temp.addCell(1000, var.annotations()[i_germl_gene] + "\\super#", RtfParagraph().setItalic(true) );

		QList<VariantTranscript> trans =  var.transcriptAnnotations(i_germl_co_sp);
		if(trans.count() > 0) temp.addCell({ trans[0].hgvs_c + ":" + trans[0].hgvs_p, RtfText(trans[0].id).setFontSize(14).RtfCode()}, 2900);
		else temp.addCell(2900, "n/a");

		temp.addCell(1700, (trans.count() > 0 ? trans[0].type.replace("_variant","") : "n/a"));

		temp.addCell(900, QByteArray::number(var.annotations()[i_germl_freq_in_tum].toDouble(), 'f', 2), RtfParagraph().setHorizontalAlignment("c"));

		//Get af of var in tumor tisse from NGSD
		temp.addCell( 3138, germlVarDesc(var.annotations()[i_germl_hom_het], var.annotations()[i_germl_class]) );

		temp_rows.append(QPair<Variant,RtfTableRow>(var,temp));
	}


	//Add somatic SNVs to temporary set
	int i_som_rep_alt = snvs.annotationIndexByName("alt_var_alteration", false);
	int i_som_rep_desc = snvs.annotationIndexByName("alt_var_description", false);
	for(int i=0; i<snvs.count(); ++i)
	{
		const Variant& snv = snvs[i];

		VariantTranscript transcript = selectSomaticTranscript(snv);
		transcript.type = transcript.type.replace("_variant","");
		transcript.type.replace("&",", ");

		RtfTableRow row;
		row.addCell(1000,transcript.gene,RtfParagraph().setItalic(true));

		//In case there is som. report annotation, overwrite protein change in RTF table
		if(i_som_rep_alt > -1 && i_som_rep_desc > -1 && (snv.annotations().at(i_som_rep_alt) != "" || snv.annotations().at(i_som_rep_desc) != ""))
		{
			row.addCell(2900, snv.annotations()[i_som_rep_alt] + ":" + snv.annotations()[i_som_rep_desc]);
		}
		else //no annotation entry from somatic report conf
		{
			row.addCell({transcript.hgvs_c + ":" + transcript.hgvs_p, RtfText(transcript.id).setFontSize(14).RtfCode()},2900);
		}

		row.last().format().setLineSpacing(276);

		row.addCell(1700,transcript.type);
		int i_tum_af = snvs.annotationIndexByName("tumor_af",true,true);
		row.addCell(900,QByteArray::number(snv.annotations().at(i_tum_af).toDouble(),'f',2),RtfParagraph().setHorizontalAlignment("c")); //tumor allele frequency

		RtfSourceCode gene_info = "unklare Funktion";
		if(!snv.annotations()[snv_index_som_class_].isEmpty())
		{
			gene_info = trans( snv.annotations()[snv_index_som_class_] );
		}
		else if(snv.annotations()[snv_index_cgi_statement_].contains("known"))
		{
			gene_info = "Treiber (bekannt)";
		}
		else if(snv.annotations()[snv_index_cgi_statement_].contains("predicted driver"))
		{
			gene_info = "Treiber (vorhergesagt)";
		}

		int i_snvs_oncogene_info = snvs.annotationIndexByName("ncg_oncogene",true,false);
		int i_snvs_tsg_info = snvs.annotationIndexByName("ncg_tsg",true,false);
		if(i_snvs_oncogene_info > -1 && i_snvs_tsg_info > -1)
		{
			if(snv.annotations().at(i_snvs_oncogene_info) == "1") gene_info.append(", Onkogen");
			if(snv.annotations().at(i_snvs_tsg_info) == "1") gene_info.append(", TSG");
		}

		row.addCell(3138, gene_info);
		temp_rows.append(QPair<Variant,RtfTableRow>(snv, row));
	}

	if(sort_by_gene)
	{
		std::sort(temp_rows.begin(), temp_rows.end(), [](const QPair<Variant,RtfTableRow>& a, const QPair<Variant,RtfTableRow>& b){return a.second[0].format().content() < b.second[0].format().content();});
	}
	else//sort according classifcation, then by gene name
	{
		std::sort(temp_rows.begin(), temp_rows.end(), [](const QPair<Variant,RtfTableRow>& a, const QPair<Variant,RtfTableRow>& b)
		{
			//germline variants
			if(a.second[0].format().content().contains("#") && b.second[0].format().content().contains("#")) return true;
			if(!a.second[0].format().content().contains("#") && b.second[0].format().content().contains("#")) return false;

			//inhouse classification
			if(a.second[4].format().content().contains("aktivierend") && !b.second[4].format().content().contains("aktivierend") ) return true;
			if(!a.second[4].format().content().contains("aktivierend") && b.second[4].format().content().contains("aktivierend") ) return false;

			//CGI known driver
			if(a.second[4].format().content().contains("bekannt") && !b.second[4].format().content().contains("bekannt") ) return true;
			if(!a.second[4].format().content().contains("bekannt") && b.second[4].format().content().contains("bekannt") ) return false;

			//CGI predicted driver
			if(a.second[4].format().content().contains("vorhergesagt") && !b.second[4].format().content().contains("vorhergesagt") ) return true;
			if(!a.second[4].format().content().contains("vorhergesagt") && b.second[4].format().content().contains("vorhergesagt") ) return false;

			//gene symbol
			if(a.second[0].format().content() < b.second[0].format().content()) return true;
			if(a.second[0].format().content() > b.second[0].format().content()) return false;

			return false; //"equal"
		}
		);
	}



	//Fill RTF table
	int i_cnv_state = cnvs_.annotationIndexByName("state", false);
	for(const auto& pair : temp_rows)
	{
		const Variant& snv = pair.first;
		const RtfTableRow& row = pair.second;

		table.addRow(row);

		//Find overlapping CNV for each SNV
		for(int i=0; i<cnvs.count(); ++i)
		{
			const CopyNumberVariant& cnv = cnvs[i];
			if(!cnv.overlapsWith(snv.chr(), snv.start(),snv.end())) continue;

			double tum_cn_change = cnv.annotations().at(cnv_index_tumor_cn_change_).toDouble();
			QByteArray gene = row[0].format().content(); //access cell with gene name
			genes_in_first_part << gene;

			RtfTableRow temp_cnv_row;
			temp_cnv_row.addCell(1000, gene, RtfParagraph().setItalic(true));


			//Add CNV information to transcript type (if cnv at this locus exists and is available)
			double tum_af = row[3].format().content().toDouble();

			double tum_maximum_clonality = getCnvMaxTumorClonality(cnvs);

			RtfText statement = RtfText("");
			if(tum_cn_change > 2)
			{
				if(tum_af < tum_maximum_clonality/2.) statement.append("AMP WT (" + cnv.annotations().at(cnv_index_tumor_cn_change_) + " Kopien)");
				else if(tum_af > tum_maximum_clonality/2.) statement.append("AMP MUT (" + cnv.annotations().at(cnv_index_tumor_cn_change_) + " Kopien)");
			}
			else if(tum_cn_change < 2)
			{
				if(tum_cn_change == 1 ) statement.append("DEL (het)");
				else if(tum_cn_change == 0) statement.append("DEL (hom)");
			}
			else
			{
				statement.append(cnv.annotations()[i_cnv_state]);
			}
			statement.setFontSize(18);

			statement.append(RtfText(cnv.chr().strNormalized(true)).setFontSize(14).RtfCode(),true); //chromosome in new line
			if(cnv_index_cytoband_ > -1)
			{
				statement.append(RtfText("; " + cytoband(cnv)).setFontSize(14).RtfCode());
			}

			temp_cnv_row.addCell(2900,statement.RtfCode());

			QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_);
			if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";
			temp_cnv_row.addCell(1700,cnv_type);

			temp_cnv_row.addCell(900,QByteArray::number(cnv.annotations().at(i_cnv_tum_clonality).toDouble(),'f',2),RtfParagraph().setHorizontalAlignment("c"));

			QByteArrayList cnv_genes = cnv.annotations().at(cnv_index_cgi_genes_).split(',');

			//Parse driver statements
			QByteArrayList driver_statements;
			for(const auto& single_gene : gene.split(','))
			{
				for(int j=0; j < cnv_genes.count() ; ++j)
				{
					if(single_gene == cnv_genes[j])
					{
						driver_statements << CnvDescription(cnv);
					}
				}
			}
			if(!driver_statements.isEmpty())
			{
				temp_cnv_row.addCell(3138, driver_statements.join(", "));
			}
			else if(statement.content().contains("LOH"))
			{
				temp_cnv_row.addCell(3138, "unklare Bedeutung / Verlust des Wildtyp-Allels",RtfParagraph().highlight(3));
			}
			else
			{
				temp_cnv_row.addCell(3138,"unklare Bedeutung");
			}

			//set first cell of corresponding cnv (contains gene name) as end of cell over multiple rows
			table.addRow(temp_cnv_row);

		}
	}


	if(!include_cnvs)	return table;

	//Add remaining CNVs to table
	int i_ncg_oncogene = cnvs_.annotationIndexByName("ncg_oncogene", false);
	int i_ncg_tsg = cnvs_.annotationIndexByName("ncg_tsg", false);

	QMap<QByteArray,RtfTableRow> cna_genes_per_row;

	//Make list of CNV drivers
	for(int i=0;i<cnvs_.count();++i)
	{
		const CopyNumberVariant& cnv = cnvs_[i];

		if(cnvs_[i].annotations()[cnv_index_cn_change_].toDouble() == 2.) continue; //Skip LOHs (for cnvs without SNV)


		QByteArrayList genes = cnv.annotations().at(cnv_index_cgi_genes_).split(',');
		QByteArrayList statements = cnv.annotations().at(cnv_index_cgi_driver_statement_).split(',');

		//ignore amplifications <4
		double tum_cn_change = cnv.annotations().at(cnv_index_tumor_cn_change_).toDouble();
		if(tum_cn_change > 2 && tum_cn_change < 4)
		{
			//make list of skipped amplified genes (driver only!)
			for(int j=0; j<genes.count(); ++j)
			{
				if(statements[j].contains("driver") || statements[j].contains("known") || target_genes.contains(genes[j]))	skipped_amp_ << genes[j];
			}
			continue;
		}


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
			RtfText cn_statement( CnvTypeDescription(cnv.annotations().at(cnv_index_tumor_cn_change_).toInt()) );
			cn_statement.append(RtfText(cnv.chr().strNormalized(true)).setFontSize(14).RtfCode(), true);

			if(cnv_index_cytoband_ > -1 ) cn_statement.append(RtfText("; " + cytoband(cnv)).setFontSize(14).RtfCode());


			QByteArray tumor_clonality = QByteArray::number(cnv.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2);

			QByteArray cnv_type = cnv.annotations().at(cnv_index_cnv_type_);
			if(!cnv_type.contains("focal") && !cnv_type.contains("cluster")) cnv_type = "non-focal";

			foreach(int index,i_genes_to_be_printed)
			{
				//Filter out genes that do not lie in target region
				genes[index] = db_.geneToApproved(genes[index],true);

				RtfTableRow row;
				row.addCell(1000,genes[index],RtfParagraph().setItalic(true));
				row.addCell(2900,cn_statement.RtfCode());
				row.addCell(1700,cnv_type);
				row.addCell(900,tumor_clonality,RtfParagraph().setHorizontalAlignment("c"));

				//Gene description
				QByteArrayList desc;
				desc << CnvDescription(cnv);

				if(i_ncg_oncogene > -1 && i_ncg_tsg > -1)
				{
					QByteArray oncogene_statement = cnv.annotations().at(i_ncg_oncogene).split(',').at(index);
					QByteArray tsg_statement = cnv.annotations().at(i_ncg_tsg).split(',').at(index);

					if(oncogene_statement == "1") desc << "Onkogen";
					if(tsg_statement == "1") desc << "TSG";
				}

				row.addCell(3138,desc.join(", "));

				cna_genes_per_row.insert(genes[index],row);
			}
		}
	}

	//Add remaining CNVs to table, sorted according gene (which is key of cna_genes_per_row QMap)
	for(const auto& row : cna_genes_per_row)
	{
		table.addRow(row);
	}

	//Merge cells with the same gene name
	for(int r=2; r<table.count()-1; ++r)
	{
		if(table[r-1][0].format().content() == table[r][0].format().content())
		{
			//set first row cell that contains gene name as cell over multiple rows as "clvmgf"
			if(table[r-1][0].controlWord().isEmpty()) table[r-1][0].setHeaderControlWord("clvmgf");

			//set second row cell as cell in center / at end of merge
			table[r][0].setHeaderControlWord("clvmrg");
		}
	}

	return table;
}


bool SomaticReportHelper::checkRequiredSNVAnnotations(const VariantList &snvs)
{
	int i_ncg_oncogene = snvs.annotationIndexByName("ncg_oncogene", true, false);
	int i_ncg_tsg = snvs.annotationIndexByName("ncg_tsg", true, false);
	int i_som_class = snvs.annotationIndexByName("somatic_classification", true, false);
	int i_cgi = snvs.annotationIndexByName("CGI_driver_statement", true, false);

	if(i_ncg_oncogene < 0 || i_ncg_tsg < 0 || i_som_class < 0 || i_cgi < 0) return false;

	return true;
}


RtfTable SomaticReportHelper::createCnvTable()
{
	RtfTable cnv_table;

	//Table Header
	cnv_table.addRow(RtfTableRow({"Chromosomale Aberrationen"},doc_.maxWidth(),RtfParagraph().setHorizontalAlignment("c").setBold(true)).setBackgroundColor(5).setHeader());
	cnv_table.addRow(RtfTableRow({"Position","CNV","Typ","CN","Anteil Probe","Gene"},{1700,1000,800,400,700,5038},RtfParagraph().setHorizontalAlignment("c").setBold(true)).setHeader());

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

	if(cnv_index_cgi_genes_ < 0 || cnv_index_cgi_driver_statement_ < 0 || cnv_index_cgi_gene_role_ < 0)
	{
		cnv_table.addRow(RtfTableRow("Fehlerhafte CGI statements in ClinCNV-Datei.",doc_.maxWidth()));
		cnv_table.setUniqueBorder(1,"brdrhair",4);
		return cnv_table;
	}

	if(cnv_index_tumor_cn_change_ < 0 || cnv_index_tumor_clonality_ < 0)
	{
		cnv_table.addRow(RtfTableRow("Die ClinCNV-Datei enthält keine Tumor Clonality. Bitte mit einer aktuelleren Version von ClinCNV neu berechnen.",doc_.maxWidth()));
		cnv_table.setUniqueBorder(1,"brdrhair",4);
		return cnv_table;
	}

	//neccessary for filtering for target region
	//create set with target genes
	QString target_region = processing_system_data.target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region.left(target_region.size()-4) + "_genes.txt");
	target_genes = db_.genesToApproved(target_genes);

	int i_cnv_state = cnvs_.annotationIndexByName("state", false);

	for(int i=0; i<cnvs_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_[i];

		RtfTableRow temp_row;

		//coordinates
		QList<RtfSourceCode> coords;
		coords << RtfText(variant.chr().str()).setFontSize(18).RtfCode();
		coords << QByteArray::number(variant.start() == 0 ? 1 : variant.start()) + " - " + QByteArray::number(variant.end());
		temp_row.addCell(coords,1700);
		temp_row.last().format().setFontSize(14);


		//AMP/DEL
		double tumor_copy_number_change = variant.annotations().at(cnv_index_tumor_cn_change_).toDouble();
		if(tumor_copy_number_change > 2.)
		{
			temp_row.addCell(1000,"AMP");
		}
		else if(tumor_copy_number_change < 2.)
		{
			QByteArray del_text = "DEL";
			if(tumor_copy_number_change == 0.) del_text += " (hom)";
			else if(tumor_copy_number_change == 1.) del_text += " (het)";
			temp_row.addCell(1000,del_text);
		}
		else
		{
			//check whether information about loss of heterocigosity is available
			if(i_cnv_state != -1 && variant.annotations().at(i_cnv_state) == "LOH")
			{
				temp_row.addCell(1000,"LOH");
			}
			else
			{
				temp_row.addCell(1000,"NA");
				temp_row.last().format().highlight(3);
			}
		}
		temp_row.last().format().setHorizontalAlignment("c");

		//Type
		RtfSourceCode type_statement = variant.annotations().at(cnv_index_cnv_type_);
		type_statement = type_statement.replace("chromosome", "chr");
		type_statement = type_statement.replace("partial p-arm", "partial\\line p-arm");
		type_statement = type_statement.replace("partial q-arm", "partial\\line q-arm");
		temp_row.addCell(800, type_statement, RtfParagraph().setHorizontalAlignment("c"));

		//copy numbers
		temp_row.addCell(400,variant.annotations().at(cnv_index_tumor_cn_change_));

		temp_row.last().format().setHorizontalAlignment("c");

		//tumor clonality
		temp_row.addCell(700,QByteArray::number(variant.annotations().at(cnv_index_tumor_clonality_).toDouble(),'f',2),RtfParagraph().setHorizontalAlignment("c"));

		//gene names
		//only print genes which lie in target region
		if(!variant.annotations().at(cnv_index_cgi_genes_).isEmpty())
		{
			//cgi genes
			QByteArrayList cgi_genes = variant.annotations().at(cnv_index_cgi_genes_).split(',');
			std::sort(cgi_genes.begin(), cgi_genes.end());

			QByteArray genes_included = "";

			for(int j=0;j<cgi_genes.count();++j)
			{
				//skip if gene is not contained in target region
				if(!target_genes.contains(cgi_genes[j])) continue;

				genes_included.append(RtfText(cgi_genes[j]).setItalic(true).RtfCode());
				genes_included.append(", ");
			}
			genes_included.truncate(genes_included.count()-2); //remove final ','
			temp_row.addCell(5038,genes_included);
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
				temp_row.addCell(5038,tmp_entry);
			}
			else
			{
				temp_row.addCell(5038, "");
			}
		}
		cnv_table.addRow(temp_row);
	}
	cnv_table.setUniqueBorder(1,"brdrhair",4);

	return cnv_table;
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

		//Skip pre-clinicals
		if(line.at(i_evidence).trimmed() == "Pre-clinical") continue;

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


SomaticReportHelper::SomaticReportHelper(const VariantList& variants, const CnvList &cnvs, const VariantList& variants_germline, const SomaticReportSettings& settings)
	: settings_(settings)
	, target_region_()
	, tumor_ps_(settings.tumor_ps)
	, normal_ps_(settings.normal_ps)
	, snv_germline_(variants_germline)
	, cnvs_()
	, validated_viruses_()
	, db_()
{
	//Filter variants according report configuration settings
	snv_variants_ = SomaticReportSettings::filterVariants(variants, settings);
	cnvs_ = SomaticReportSettings::filterCnvs(cnvs, settings);

	//Sort Variant list by gene
	snv_variants_.sortByAnnotation(snv_variants_.annotationIndexByName("gene",true,true));

	if(settings.report_config.targetFile() != "") //resolve target region in sub-panel directory
	{
		target_region_ = Helper::canonicalPath(db_.getTargetFilePath(true,true) + "/" + QFileInfo(settings.report_config.targetFile()).fileName());
	}

	QString prefix = tumor_ps_ + "-" + normal_ps_;

	snv_filename_ = Helper::canonicalPath(settings_.sample_dir + "/" + prefix + ".GSvar");

	//cgi drug prescriptions
	cgi_drugs_path_ = Helper::canonicalPath(settings_.sample_dir + "/" + prefix + "_cgi_drug_prescription.tsv");

	//filename MANTIS output
	mantis_msi_path_ = Helper::canonicalPath(settings_.sample_dir + "/" + prefix + "_msi.tsv");

	try
	{
		TSVFileStream msi_file(mantis_msi_path_);
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
	try
	{
		QString path = db_.processedSamplePath(db_.processedSampleId(tumor_ps_), NGSD::PathType::SAMPLE_FOLDER) + "/" + tumor_ps_ + "_viral.tsv";
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

			if(tmp.coverage_ < 100) continue;
			if(tmp.idendity_ < 90) continue;

			validated_viruses_ << tmp;
		}
	}
	catch(...)
	{
		;//nothing to do here
	}

	//load obo terms for filtering coding/splicing variants
	OntologyTermCollection obo_terms("://Resources/so-xp_3_0_0.obo",true);
	QList<QByteArray> ids;
	ids << obo_terms.childIDs("SO:0001580",true); //coding variants
	ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
	foreach(const QByteArray& id, ids)
	{
		obo_terms_coding_splicing_.add(obo_terms.findByID(id));
	}

	//Assign SNV annotation indices
	snv_index_som_class_ = snv_variants_.annotationIndexByName("somatic_classification");
	snv_index_coding_splicing_ = snv_variants_.annotationIndexByName("coding_and_splicing");
	snv_index_cgi_statement_ = snv_variants_.annotationIndexByName("CGI_driver_statement");

	//assign CNV annotation indices
	cnv_index_cn_change_ = cnvs_.annotationIndexByName("CN_change", false);
	cnv_index_cgi_gene_role_ = cnvs_.annotationIndexByName("CGI_gene_role", false);
	cnv_index_cnv_type_ = cnvs_.annotationIndexByName("cnv_type", false);
	cnv_index_cgi_genes_ = cnvs_.annotationIndexByName("CGI_genes", false);
	cnv_index_cgi_driver_statement_ = cnvs_.annotationIndexByName("CGI_driver_statement", false);
	cnv_index_tumor_clonality_ = cnvs_.annotationIndexByName("tumor_clonality", false);
	cnv_index_tumor_cn_change_ = cnvs_.annotationIndexByName("tumor_CN_change", false);

	cnv_index_cytoband_ = cnvs.annotationIndexByName("cytoband", false);


	//load qcml data
	qcml_data_ = QCCollection::fromQCML(Helper::canonicalPath(settings_.sample_dir + "/" +prefix + "_stats_som.qcML"));

	processing_system_data = db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(tumor_ps_), true);

	//load disease details from NGSD
	QStringList tmp;
	QList<SampleDiseaseInfo> disease_info = db_.getSampleDiseaseInfo(db_.sampleId(tumor_ps_));

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
		QString mb_string = qcml_data_.value("QC:2000053",true).toString();
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

	//load list with CGI acronyms, CGI explanation and TMB
	cgi_dictionary_ = cgi_info::load("://Resources/cancer_types.tsv");

	//Set up RTF file specifications
	doc_.setMargins(1134,1134,1134,1134);
	doc_.addColor(188,230,138);
	doc_.addColor(255,0,0);
	doc_.addColor(255,255,0);
	doc_.addColor(161,161,161);
	doc_.addColor(217,217,217);
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

QMap<QByteArray, BedFile> SomaticReportHelper::gapStatistics(const BedFile& region_of_interest)
{
	BedFile roi_inter;
	ProcessingSystemData system_data = db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(tumor_ps_), true);
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

	GeneSet genes_in_target_region;
	if(!target_region_.isEmpty()) genes_in_target_region = GeneSet::createFromFile(target_region_.left(target_region_.size()-4)+"_genes.txt");

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
		widths << 700 << 700 << 900 << 7337;
		table.addRow(RtfTableRow({"Gen","Lücken","Chromosom","Koordinaten (GRCh37)"},widths,RtfParagraph().setBold(true).setFontSize(16).setSpaceBefore(50)));

		for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
		{
			const BedFile& gaps = it.value();
			QByteArray chr = gaps[0].chr().strNormalized(true);
			QByteArrayList coords;
			for (int i=0; i<gaps.count(); ++i)
			{
				coords << QByteArray::number(gaps[i].start()) + "\\_" + QByteArray::number(gaps[i].end());
			}
			table.addRow(RtfTableRow({RtfText(it.key()).setItalic(true).setFontSize(14).RtfCode(),QByteArray::number(gaps.baseCount()),chr,coords.join(", ")},widths,RtfParagraph()));
		}
	}

	table.setUniqueFontSize(14);
	return table;
}


RtfTable SomaticReportHelper::createCgiDrugTable()
{
	RtfTable cgi_table;

	RtfParagraph header_format;
	header_format.setBold(true);
	header_format.setIndent(20,0,0);
	header_format.setFontSize(16);
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
	for(int i=0;i<cnvs_.count();++i)
	{
		keep_cnv_genes << cnvs_[i].genes();
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

		//remove brackets in drug name
		result_line[2].replace("(","");
		result_line[2].replace(")","");

		cgi_table.addRow(RtfTableRow(result_line,{1500,1200,2300,1000,1000,2638},RtfParagraph().setIndent(20,0,0).setFontSize(14)));

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
		QByteArray effect;

		if(variant.annotations().at(snv_index_som_class_) == "activating" || variant.annotations().at(snv_index_som_class_) == "likely_activating")
		{
			effect = "activating";
		}
		else if(variant.annotations().at(snv_index_som_class_) == "inactivating" || variant.annotations().at(snv_index_som_class_) == "likely_inactivating")
		{
			effect = "inactivating";
		}
		else if(variant.annotations().at(snv_index_som_class_) == "test_dependent")
		{
			effect = "ambiguous";
		}
		else
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

	QString target_region_processing_system = db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(tumor_ps_), true).target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region_processing_system.left(target_region_processing_system.size()-4) + "_genes.txt");
	NGSD db;
	target_genes = db.genesToApproved(target_genes);

	for(int i=0; i < cnvs_.count(); ++i)
	{
		const CopyNumberVariant& variant = cnvs_[i];

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

	stream << db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(tumor_ps_), true).genome;
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

QByteArray SomaticReportHelper::CnvDescription(const CopyNumberVariant& cnv)
{
	if(cnv.annotations()[cnv_index_cn_change_].toDouble() > 2.) return "mögl. Überexpression";
	else if(cnv.annotations()[cnv_index_cn_change_].toDouble() < 2.) return "mögl. reduzierte Expression";
	return "unklare Bedeutung";
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

RtfTable SomaticReportHelper::pharamacogeneticsTable()
{
	RtfTable table;

	const static QMultiMap< QByteArray, QPair<QByteArray,QByteArray> > data = {
		{"rs1142345",QPair<QByteArray,QByteArray>("Toxicity","Cisplatin")},
		{"rs1142345",QPair<QByteArray,QByteArray>("Efficacy","Cisplatin, Cyclophosphamide")},
		{"rs12248560",QPair<QByteArray,QByteArray>("Toxicity","Cyclophosphamid, Doxorubicin, Fluoruracil")},
		{"rs1800460",QPair<QByteArray,QByteArray>("Toxicity","Cisplatin")},
		{"rs3745274",QPair<QByteArray,QByteArray>("Dosage","Cyclophosphamide, Doxorubicin")},
		{"rs3892097",QPair<QByteArray,QByteArray>("Efficacy, Toxicity","Tamoxifen")},
		{"rs35742686", QPair<QByteArray, QByteArray>("Metabolism", "Tamoxifen")},
		{"rs3918290",QPair<QByteArray,QByteArray>("Efficacy","Fluoruracil")},
		{"rs3918290",QPair<QByteArray,QByteArray>("Toxicity, Metabolism","Capecitabine, Fluoruracil, Pyrimidine analogues, Tegafur")},
		{"rs4148323",QPair<QByteArray,QByteArray>("Dosage","Irinotecan")},
		{"rs4148323",QPair<QByteArray,QByteArray>("Other","SN-38 (irinotecan metabolite)")},
		{"rs4148323",QPair<QByteArray,QByteArray>("Other","Irinotecan")},
		{"rs4149056",QPair<QByteArray,QByteArray>("Toxicity","Irinotecan")},
		{"rs4149056",QPair<QByteArray,QByteArray>("Toxicity","Cyclophosphamid, Docetaxel, Doxorubicin, Epirubicin, Fluoruracil")},
		{"rs4244285",QPair<QByteArray,QByteArray>("Toxicity","Cyclophosphamid, Doxorubicin")},
		{"rs4244285",QPair<QByteArray,QByteArray>("Efficacy","Cyclophosphamid, Doxorubicin")},
		{"rs4244285",QPair<QByteArray,QByteArray>("Metabolism","Nelfinavir")},
		{"rs55886062",QPair<QByteArray,QByteArray>("Toxicity","Capecitabine, Fluoruracil, Pyrimidine analogues, Tegafur")},
		{"rs56038477",QPair<QByteArray,QByteArray>("Toxicity","Capecitabine, Fluoruracil")},
		{"rs67376798",QPair<QByteArray,QByteArray>("Toxicity, Metabolism","Capecitabine, Fluoruracil, Pyrimidine analogues, Tegafur")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Toxicity","irinotecan")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Other","SN-38 (irinotecan metaboite)")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Dosage","Irinotecan")},
		{"rs8175347",QPair<QByteArray,QByteArray>("Metabolism","Belinostat")}
	};

	int i_dbsnp = snv_germline_.annotationIndexByName("dbSNP",true,false);
	int i_co_sp = snv_germline_.annotationIndexByName("coding_and_splicing",true,false);
	int i_genotype = snv_germline_.getSampleHeader().infoByStatus(true).column_index;

	if(i_dbsnp == -1)
	{
		return table;
	}


	for(int i=0;i<snv_germline_.count();++i)
	{
		const Variant& snv = snv_germline_[i];

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
						row.addCell(1800,trans.hgvs_c + ":" + trans.hgvs_p,RtfParagraph().setFontSize(14));
					}
					else //use genomic position if no AA change available
					{
						row.addCell(1800, "g." + QByteArray::number(snv.start()) + snv.ref() + ">" + snv.obs(), RtfParagraph().setFontSize(14) );
					}

					row.addCell(800,snv.annotations().at(i_genotype),RtfParagraph().setFontSize(14));
					row.addCell(1300,value.first,RtfParagraph().setFontSize(14));
					row.addCell(3737,value.second,RtfParagraph().setFontSize(14));
					table.addRow(row);
				}
			}
		}
	}

	if(table.count() != 0) //Set style and header row in case we have found SNPs
	{
		table.prependRow(RtfTableRow({"RS-Nummer","Gen","Veränderung","Genotyp","Relevanz","Assoziierte Stoffe"},{1200,800,1800,800,1300,3737},RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());
		table.prependRow(RtfTableRow({"Pharmakogenetisch relevante Polymorphismen"},doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(5).setHeader());
		table.setUniqueBorder(1,"brdrhair",4);
		table.addRow(RtfTableRow("Nähere Informationen erhalten Sie aus der Datenbank pharmGKB (https://www.pharmgkb.org)",{doc_.maxWidth()},RtfParagraph().setFontSize(14)));
	}

	return table;
}


void SomaticReportHelper::writeRtf(const QByteArray& out_file)
{
	/**********************************
	 * MUTATION BURDEN AND MSI STATUS *
	 **********************************/
	RtfTable general_info_table;

	general_info_table.addRow(RtfTableRow("Allgemeine genetische Charakteristika",doc_.maxWidth(),RtfParagraph().setBold(true)).setBackgroundColor(5).setBorders(1,"brdrhair",4));


	QByteArray tumor_content_bioinf = "";

	if(settings_.report_config.tumContentByClonality()) tumor_content_bioinf = QByteArray::number(getCnvMaxTumorClonality(cnvs_) * 100., 'f', 1) + " \%";
	if(settings_.report_config.tumContentByMaxSNV())
	{
		double tumor_molecular_proportion;
		try
		{
			 tumor_molecular_proportion = qcml_data_.value("QC:2000054",true).toString().toDouble();
		}
		catch(...)
		{
			tumor_molecular_proportion = std::numeric_limits<double>::quiet_NaN();
		}

		if(tumor_molecular_proportion > 100.)	tumor_molecular_proportion = 100.;

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
		tumor_content_hist = QByteArray::number(histol_tumor_fraction_, 'f', 1) + " \%";
	}
	general_info_table.addRow(RtfTableRow({"Tumoranteil (hist./bioinf.)", tumor_content_hist + " / " + tumor_content_bioinf}, {2500, 7137}).setBorders(1,"brdrhair", 4));


	RtfText mutation_burden_text(QByteArray::number(mutation_burden_) + " Var/Mbp");
	if(settings_.report_config.tmbReferenceText() != "")
	{
		mutation_burden_text.append(";");
		mutation_burden_text.append(RtfText("Vergleichswerte: " + settings_.report_config.tmbReferenceText().toUtf8()).setFontSize(14).RtfCode(), true);
	}

	general_info_table.addRow(RtfTableRow({"Mutationslast:", mutation_burden_text.RtfCode()}, {2500,7137}).setBorders(1,"brdrhair",4));
	general_info_table.last()[0].setBorder(1,1,1,0,"brdrhair");
	general_info_table.last().last().setBorder(1,1,1,0,"brdrhair");


	//MSI status, values larger than 0.16 are considered unstable
	if(settings_.report_config.msiStatus())
	{
		general_info_table.addRow(RtfTableRow({"Mikrosatelliten:", ( mantis_msi_swd_value_ <= 0.16 ? "kein Hinweis auf eine MSI" : "Hinweise auf MSI" ) },{2500,7137}).setBorders(1,"brdrhair",4));
	}

	//Fusion status
	general_info_table.addRow(RtfTableRow({"Fusionen/Strukturvarianten", (settings_.report_config.fusionsDetected() ? "nachgewiesen" : "nicht nachgewiesen")}, {2500,7137}).setBorders(1, "brdrhair", 4));

	//Virus DNA status
	QByteArrayList virus_names;
	for(const auto& virus : validated_viruses_)
	{
		virus_names << virus.virusName();
	}
	general_info_table.addRow(RtfTableRow({"Virus-DNA", (virus_names.count() > 0 ? virus_names.join(", ") + " nachgewiesen" : "nicht nachgewiesen")}, {2500,7137}).setBorders(1, "brdrhair", 4));


	//Calc percentage of CNV altered genome

	if(settings_.report_config.cnvBurden())
	{
		RtfSourceCode text_cnv_burden = "";
		double cnv_altered_percentage = cnvBurden(cnvs_);
		if(cnv_altered_percentage >= 0.01)
		{
			text_cnv_burden = QByteArray::number(cnv_altered_percentage,'f',1) + " \%" ;
		}
		else
		{
			text_cnv_burden =  "Keine CNVs nachgewiesen";
		}

		if(settings_.report_config.cinChromosomes().count() > 0)
		{
			QList<QString> chr = settings_.report_config.cinChromosomes();

			//Sort chromosomes naturally
			QCollator coll;
			coll.setNumericMode(true);
			std::sort(chr.begin(), chr.end(), [&](const QString s1, const QString& s2){return coll.compare(s1,s2) < 0;});

			RtfSourceCode temp = "\\line Es gibt Hinweise auf eine chromosomale Instabilität auf Chr. ";
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
			text_cnv_burden += RtfText("\\line Es gibt keine Hinweise auf chromosomale Instabilität.").setFontSize(14).RtfCode();
		}

		general_info_table.addRow(RtfTableRow({"CNV-Last:", text_cnv_burden},{2500,7137},RtfParagraph()).setBorders(1,"brdrhair",4));
	}

	QByteArray hrd_text;
	if(settings_.report_config.hrdScore() > 0) hrd_text = QByteArray::number(settings_.report_config.hrdScore());
	else hrd_text = "nicht nachgewiesen";
	general_info_table.addRow(RtfTableRow({"HRD-Score:", hrd_text}, {2500,7137},  RtfParagraph()).setBorders(1, "brdrhair", 4));

	if(settings_.report_config.quality() != "no abnormalities")
	{
		general_info_table.addRow(RtfTableRow({"Kommentar", trans(settings_.report_config.quality()).toUtf8()}, {2500, 7137}, RtfParagraph()).setBorders(1, "brdrhair", 4));
	}


	doc_.addPart(general_info_table.RtfCode());
	doc_.addPart(RtfParagraph("").RtfCode());


	/********************************
	 * RELEVANT SOMATIC ALTERATIONS *
	 ********************************/
	doc_.addPart(RtfParagraph("Potentiell relevante somatische Veränderungen:").setBold(true).setIndent(0,0,0).setSpaceBefore(250).RtfCode());

	VariantList snvs_to_be_printed;
	snvs_to_be_printed.copyMetaData(snv_variants_);

	QBitArray pass(snv_variants_.count(),false);

	if(target_region_.isEmpty()) //Include variants with in-house somatic classification only
	{
		for(int i=0;i<snv_variants_.count();++i)
		{
			const QByteArray& som_class = snv_variants_[i].annotations().at(snv_index_som_class_);
			if(!som_class.isEmpty()) //in house classification overwrites CGI
			{
				if(som_class != "n/a" && som_class != "unclear") pass[i] = true;
			}
			else //fallback to CGI
			{
				const QByteArray& cgi = snv_variants_[i].annotations().at(snv_index_cgi_statement_);
				if(cgi.contains("known") || cgi.contains("predicted driver")) pass[i] = true;
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

	//Create list with genes in target region
	QString target_genes_file = target_region_.left(target_region_.size()-4) + "_genes.txt";
	GeneSet target_genes;
	if(!target_region_.isEmpty() && QFile::exists(target_genes_file))
	{
		 target_genes = GeneSet::createFromFile(target_genes_file);
		 target_genes = db_.genesToApproved(target_genes,true);
	}

	doc_.addPart(somaticAlterationTable(snvs_to_be_printed, cnvs_, true, target_genes, true).setUniqueBorder(1,"brdrhair",4).RtfCode());

	RtfSourceCode snv_expl  = RtfText("Anteil:").setBold(true).setFontSize(14).RtfCode() + " Anteil der Allele mit der gelisteten Variante (SNV, INDEL) bzw. Anteil der Zellen mit der entsprechenden Kopienzahlvariante (CNV) in der untersuchten Probe. ";
	snv_expl += RtfText("Beschreibung:").setFontSize(14).setBold(true).RtfCode() + " SNV: Einschätzung der Varianten und ggf. Bewertung der Genfunktion als Onkogen bzw. Tumorsuppressorgen (TSG). CNV: Deletionen, fokale Amplifikationen und Amplifikationen mit mehr als 4 Kopien. ";
	snv_expl += "Erweiterte Legende und Abkürzungen siehe unten.";
	if(settings_.report_config.countGermline() > 0) snv_expl += "\\line\n" +RtfText("#:").setFontSize(14).setBold(true).RtfCode() + " Auch in der Normalprobe nachgewiesen.";

	doc_.addPart( RtfParagraph(snv_expl).setFontSize(14).setIndent(0,0,0).setHorizontalAlignment("j").setLineSpacing(276).RtfCode() );
	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setFontSize(18).RtfCode());

	if(skipped_amp_.count() > 0)
	{
		snv_expl = "Weiterhin wurden nicht-fokale Amplifikationen (3 Kopien) für die folgenden relevanten Gene nachgewiesen: ";
		std::sort(skipped_amp_.begin(), skipped_amp_.end());
		snv_expl += RtfText(skipped_amp_.join(", ")).setFontSize(18).setItalic(true).RtfCode();
		snv_expl += ".";
		doc_.addPart(RtfParagraph(snv_expl).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());
		doc_.addPart(RtfParagraph("").setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setFontSize(18).RtfCode());
	}

	if(settings_.report_config.fusionsDetected())
	{
		snv_expl = "Es gibt Hinweise auf eine Deletion/Fusion/Translokation/Strukturvariante, die zu einer Fusion/Deletion/... führen könnte. Eine Bestätigung dieser Beobachtung sollte mit einer unabhängigen Methode erfolgen.";
		doc_.addPart(RtfParagraph(snv_expl).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setBold(true).highlight(3).RtfCode());
		doc_.addPart(RtfParagraph("").setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setLineSpacing(276).setFontSize(18).RtfCode());
	}



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
	snv_expl += "\\line Die Varianten- und Gendosisanalysen der Gene " + RtfText("BRCA1").setItalic(true).setFontSize(18).RtfCode() + " und " + RtfText("BRCA2").setItalic(true).setFontSize(18).RtfCode();
	snv_expl += " in der Normalprobe waren unauffällig.";
	snv_expl += "\\line Eine Analyse des Transkriptoms bei dieser Tumorprobe wird nachträglich berichtet.";
	snv_expl += "\\line Eine somatische Analyse einer zusätzlichen Tumorprobe wird nachträglich berichtet.";

	doc_.addPart(RtfParagraph(snv_expl).highlight(3).setFontSize(18).setIndent(0,0,0).setSpaceAfter(30).setSpaceBefore(30).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());

	doc_.addPart(RtfParagraph("").setIndent(0,0,0).setLineSpacing(276).setFontSize(18).RtfCode());

	doc_.newPage();

	/*********************
	 * ADDITIONAL REPORT *
	 *********************/
	doc_.addPart(RtfParagraph("Alle nachgewiesenen somatischen Veränderungen:").setBold(true).setSpaceAfter(45).setFontSize(18).RtfCode());

	doc_.addPart(somaticAlterationTable(snv_variants_, cnvs_, true).setUniqueBorder(1,"brdrhair",4).RtfCode());

	RtfSourceCode desc = "Diese Tabelle enthält sämtliche in der Tumorprobe nachgewiesenen SNVs und INDELs, unabhängig von der funktionellen Einschätzung und der abzurechnenden Zielregion. Sie enthält ferner alle Kopienzahlveränderungen in Genen, die als Treiber eingestuft wurden.";
	if(settings_.report_config.countGermline() > 0) desc += "\\line\n" +RtfText("#:").setFontSize(14).setBold(true).RtfCode() + " Auch in der Normalprobe nachgewiesen.";

	doc_.addPart(RtfParagraph(desc).setFontSize(14).setIndent(0,0,0).setHorizontalAlignment("j").setLineSpacing(276).RtfCode());

	doc_.addPart(RtfParagraph(" ").RtfCode());

	//Make rtf report, filter genes for processing system target region
	doc_.addPart(createCnvTable().RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());

	/***********
	 * FUSIONS *
	 ***********/
	if(settings_.report_config.fusionsDetected())
	{
		RtfTable fusion_table;
		fusion_table.addRow(RtfTableRow("Strukturvarianten",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setHeader().setBackgroundColor(5));
		fusion_table.addRow(RtfTableRow({"Region A", "Region B", "Beschreibung"}, {2409, 2409, 4818}, RtfParagraph().setBold(true).setHorizontalAlignment("c").setFontSize(16)).setHeader());
		fusion_table.addRow(RtfTableRow({"", "", "", "", ""}, {1000,1409,1000,1409,4818}, RtfParagraph().setFontSize(16)));

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
		virus_table.addRow(RtfTableRow("Virale DNA",doc_.maxWidth(),RtfParagraph().setBold(true).setHorizontalAlignment("c")).setBackgroundColor(5));
		virus_table.addRow(RtfTableRow({"Virus","Gen","Genom","Region","Abdeckung","Bewertung"},{963,964,1927,1927,1927,1929},RtfParagraph().setBold(true)));
		for(const auto& virus : validated_viruses_)
		{
			RtfTableRow row;

			row.addCell(963,virus.virusName());

			row.addCell(964,virus.virusGene());
			row.addCell(1927,virus.chr_);

			QByteArray region = QByteArray::number(virus.start_) + "-" + QByteArray::number(virus.end_);
			row.addCell(1927,region);
			row.addCell(1927,QByteArray::number(virus.coverage_,'f',1));
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

	/******************
	 * CGI DRUG TABLE *
	 ******************/
	doc_.addPart(createCgiDrugTable().setUniqueBorder(1,"brdrhair",4).RtfCode());

	/*********************
	 * CGI ACRONYMS LIST *
	 *********************/
	RtfTable cgi_acronyms_table;

	QByteArrayList cgi_acronym_explanation;
	for(const cgi_info& data : cgi_dictionary_)
	{
		if(cgi_acronyms_.contains(data.acronym)) cgi_acronym_explanation << data.acronym + ": " + data.def_german;
	}
	cgi_acronyms_table.addRow(RtfTableRow(cgi_acronym_explanation.join(", "),doc_.maxWidth(), RtfParagraph().setFontSize(14).setHorizontalAlignment("j").setLineSpacing(276)));

	doc_.addPart(cgi_acronyms_table.RtfCode());

	doc_.addPart(RtfParagraph("").RtfCode());


	/*******************************************
	 * GENERAL INFORMATION / QUALITY PARAMETERS*
	 *******************************************/
	QDir directory = QFileInfo(snv_filename_).dir();
	directory.cdUp();
	QString qcml_file_tumor_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + tumor_ps_ + "/" + tumor_ps_ + "_stats_map.qcML";
	QString qcml_file_normal_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + normal_ps_ + "/" + normal_ps_ + "_stats_map.qcML";

	QCCollection qcml_data_tumor = QCCollection::fromQCML(qcml_file_tumor_stats_map_absolute_path);
	QCCollection qcml_data_normal = QCCollection::fromQCML(qcml_file_normal_stats_map_absolute_path);


	RtfTable metadata;
	metadata.addRow(RtfTableRow({RtfText("Allgemeine Informationen").setBold(true).setFontSize(16).RtfCode(),RtfText("Qualitätsparameter").setBold(true).setFontSize(16).RtfCode()}, {4550,5087}));
	metadata.addRow(RtfTableRow({"Datum:",QDate::currentDate().toString("dd.MM.yyyy").toUtf8(),"Analysepipeline:", snv_variants_.getPipeline().toUtf8()}, {1550,3000,2250,2837}));
	metadata.addRow(RtfTableRow({"Proben-ID (Tumor):", tumor_ps_.toUtf8(), "Auswertungssoftware:", QCoreApplication::applicationName().toUtf8() + " " + QCoreApplication::applicationVersion().toUtf8()},{1550,3000,2250,2837}));
	metadata.addRow(RtfTableRow({"Proben-ID (Keimbahn):", normal_ps_.toUtf8(), "Coverage Tumor 100x:", qcml_data_tumor.value("QC:2000030",true).toString().toUtf8() + " \%"},{1550,3000,2250,2837}));
	metadata.addRow(RtfTableRow({"CGI-Tumortyp:", cgi_cancer_type_.toUtf8(), "Durchschnittliche Tiefe Tumor:", qcml_data_tumor.value("QC:2000025",true).toString().toUtf8() + "x"},{1550,3000,2250,2837}));


	metadata.addRow(RtfTableRow({"MSI-Status:", (!BasicStatistics::isValidFloat(mantis_msi_swd_value_) ? "n/a" : QByteArray::number(mantis_msi_swd_value_,'f',3)), "Coverage Normal 100x:", qcml_data_normal.value("QC:2000030",true).toString().toUtf8() + " \%"} , {1550,3000,2250,2837} ));

	GeneSet gene_set = GeneSet::createFromFile(processing_system_data.target_file.left(processing_system_data.target_file.size()-4) + "_genes.txt");
	metadata.addRow(RtfTableRow({"Prozessierungssystem:",processing_system_data.name.toUtf8() + " (" + QByteArray::number(gene_set.count()) + ")", "Durchschnittliche Tiefe Normal:", qcml_data_normal.value("QC:2000025",true).toString().toUtf8() + "x"},{1550,3000,2250,2837}));

	metadata.setUniqueFontSize(14);

	doc_.addPart(metadata.RtfCode());

	/******************
	 * GAP STATISTICS *
	 ******************/
	if(!target_region_.isEmpty()) //Only for EBM report: gap statistics
	{
		doc_.addPart(RtfParagraph("").RtfCode());
		doc_.addPart(RtfParagraph("Lückenstatistik:").setBold(true).setSpaceAfter(45).setSpaceBefore(45).setFontSize(16).RtfCode());
		doc_.addPart(createGapStatisticsTable({1550,8087}).RtfCode());
	}
	doc_.save(out_file);
}


QString SomaticReportHelper::trans(const QString &text) const
{
	QHash<QString, QString> en2de;
	en2de["no abnormalities"] = "keine Auffälligkeiten";
	en2de["tumor cell content too low"] = "Tumorzellgehalt zu niedrig";
	en2de["quality of tumor DNA too low"] = "Qualität der Tumor-DNA zu gering";
	en2de["DNA quantity too low"] = "DNA-Menge im Tumor zu gering";
	en2de["activating"] = "aktivierend";
	en2de["likely_activating"] = "möglicherweise aktivierend";
	en2de["inactivating"] = "inaktivierend";
	en2de["likely_inactivating"] = "möglicherweise inaktivierend";
	en2de["unclear"] = "unklare Bedeutung";
	en2de["test_dependent"] = "testabhängige Bedeutung";

	if(!en2de.contains(text)) return text; //return original entry if not found

	return en2de[text];
}
