#include "ReportHelper.h"
#include "BasicStatistics.h"
#include "OntologyTermCollection.h"
#include "VariantFilter.h"
#include "Helper.h"
#include "GenLabDB.h"
#include "TSVFileStream.h"
#include <cmath>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMainWindow>
#include <QSqlError>

void RtfTools::writeRtfHeader(QTextStream& stream)
{
	//stream.setCodec("ANSI");
	stream << "{\\rtf1\\ansi\\ansicpg1252\\deff0{\\fonttbl{\\f0 Arial;}}{\\colortbl;\\red188\\green230\\blue138;\\red255\\green0\\blue0;\\red255\\green225\\blue0;}" << "\n";
	stream << "\\paperw11905\\paperh15840\\margl1134\\margr850\\margt1134\\margb1134" << "\n";
}

void RtfTools::writeRtfTableSingleRowSpec(QTextStream& stream,const QList<int>& col_widths, bool border)
{
	int col_numbers = col_widths.size();
	stream << "{\\trowd\\trql";
	if(border == true)
	{
		//Generate cells with widths out of colWidths
		for(int column=0;column<col_numbers;column++)
		{
			stream << "\\clbrdrt\\brdrw15\\brdrs\\clbrdrl\\brdrw15\\brdrs\\clbrdrb\\brdrw15\\brdrs\\clbrdrr\\brdrw15\\brdrs\\cellx" <<col_widths[column];
		}
		stream << endl;
	}
	else
	{
		//Generate cells with widths out of colWidths
		for(int column=0;column<col_numbers;column++)
		{
			stream << "\\cellx" <<col_widths[column];
		}
		stream << endl;
	}
}

void RtfTools::writeRtfWholeTable(QTextStream& stream, const QList< QList<QString> >& table, const QList<int>& col_widths, int font_size, bool border, bool bold)
{
	int rows = table.length();
	for(int i=0;i<rows;i++)
	{
		RtfTools::writeRtfRow(stream, table.at(i), col_widths, font_size, border, bold);
	}
}

void RtfTools::writeRtfRow(QTextStream& stream, const QList<QString>& columns, const QList<int>& col_widths, int font_size, bool border, bool bold)
{
	QString begin_table_cell = "\\pard\\intbl\\fs" + QString::number(font_size) + " ";
	if(bold)
	{
		begin_table_cell = begin_table_cell + "\\b ";
	}

	RtfTools::writeRtfTableSingleRowSpec(stream,col_widths,border);
	for(int i=0;i<columns.length();i++)
	{
		stream << begin_table_cell << columns.at(i) << "\\cell";
	}
	stream << "\\row}" << endl;
}

void ReportHelper::writeRtfTableSNV(QTextStream& stream, const QList<int>& colWidths, bool display_germline_hint)
{
	VariantList important_snvs;

	if(display_germline_hint)
	{
		important_snvs = filterSnvForCGIAnnotation(false);
	}
	else
	{
		important_snvs = filterSnvForCGIAnnotation(true);
	}

	//set widths
	int max_table_width = colWidths.last();

	QList<int> widths;

	QString begin_table_cell = "\\pard\\intbl\\fs18 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";

	//annotation indices
	int i_tum_dp = important_snvs.annotationIndexByName("tumor_dp",true,true);
	int i_tum_af = important_snvs.annotationIndexByName("tumor_af",true,true);

	widths = colWidths;

	//rtf somatic SNV table
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);

	stream << "\\trhdr" << begin_table_cell_bold << "\\qc Gen\\cell" << begin_table_cell_bold << "\\qc cDNA\\cell";
	stream << begin_table_cell_bold << "\\qc Typ\\cell" << begin_table_cell_bold << "\\qc F/T Tumor\\cell" << begin_table_cell_bold << "\\qc Funktion\\cell" << begin_table_cell_bold<<"\\qc Effekt\\cell" << "\\row}" << endl;


	widths.clear();
	widths << max_table_width;

	//list with cancer acronyms that occur in SNV table
	QByteArrayList cancer_acronyms;

	//construct 2D QStringList to create RTF table
	QList< QList<QString> > somatic_snv_table_unordered;
	for(int i=0;i<important_snvs.count();i++)
	{
		QList<QString> columns;
		//gene
		columns.append("\\ql\\li20 "+ important_snvs[i].annotations().at(snv_index_cgi_gene_));

		//cDNA
		//Only print one transcript: that one from CGI, otherwise the first transcript in the columns
		QByteArrayList hgvs_terms = important_snvs[i].annotations().at(snv_index_coding_splicing_).split(',');
		QStringList tmp_ensemble_names;

		QString cgi_transcript = important_snvs[i].annotations().at(snv_index_cgi_transcript_);
		cgi_transcript = cgi_transcript.remove(',');
		cgi_transcript = cgi_transcript.trimmed();

		//keep first transcript in colum if there is no cgi ensemble name
		int number_keep = 0;

		for(int j=0;j<hgvs_terms.length();j++)
		{
			tmp_ensemble_names.append(hgvs_terms[j].split(':')[1]);
		}

		for(int j=0;j<tmp_ensemble_names.length();j++)
		{
			if(tmp_ensemble_names[j] == cgi_transcript)
			{
				number_keep = j;
			}
		}

		//check whether chosen transcript is coding or splicing (in rare cases CGI effect prediction differs from SnpEff)
		QByteArrayList transcripts = important_snvs[i].annotations().at(snv_index_coding_splicing_).split(',');
		QByteArrayList variant_types = transcripts[number_keep].split(':')[2].split('&');
		bool is_coding_splicing = false;
		foreach(QByteArray variant_type,variant_types)
		{
			if(obo_terms_coding_splicing_.containsByName(variant_type))
			{
				is_coding_splicing = true;
				break;
			}
		}

		//if CGI transcript is not coding/splicing, take first coding/splicing transcript from our own annotation
		if(!is_coding_splicing)
		{
			for(int j=0;j<transcripts.count();j++)
			{
				QByteArrayList single_transcript_effects = transcripts[j].split(':')[2].split('&');
				bool transcript_found = false;
				foreach(QByteArray single_effect,single_transcript_effects)
				{
					if(obo_terms_coding_splicing_.containsByName(single_effect))
					{
						number_keep = j;
						transcript_found = true;
						break;
					}
				}
				if(transcript_found) break;
			}

		}

		bool is_cgi_transcript;
		if(cgi_transcript == transcripts[number_keep].split(':')[1])
		{
			is_cgi_transcript = true;
		}
		else
		{
			is_cgi_transcript = false;
		}


		QString mutation = important_snvs[i].annotations().at(snv_index_coding_splicing_).split(',')[number_keep].split(':')[5];

		//for deletions: remove bases according to HGVS nomenclature
		if(mutation.contains("del"))
		{
			int pos = mutation.indexOf("del")+3;
			mutation.truncate(pos);
		}

		QString protein_change = important_snvs[i].annotations().at(snv_index_coding_splicing_).split(',')[number_keep].split(':')[6];
		if(protein_change.isEmpty())
		{
			protein_change = "p.?";
		}

		QString cdna = mutation + ":" + protein_change + "\\line " + "(" +important_snvs[i].annotations().at(snv_index_coding_splicing_).split(',')[number_keep].split(':')[1] + ")";
		columns.append("\\li20 " + cdna);

		//type: print coding and splicing only
		QByteArray types_all = important_snvs[i].annotations().at(snv_index_coding_splicing_).split(',')[number_keep].split(':')[2];

		types_all = types_all.replace("_variant","");
		types_all = types_all.replace("&", ", ");

		columns.append("\\li20 " + types_all);

		//F/T Tumor
		columns.append("\\qc " + QString::number(important_snvs[i].annotations().at(i_tum_af).toDouble(), 'f', 3) + " / " + important_snvs[i].annotations().at(i_tum_dp));

		//Parse driver statement
		QByteArray driver_statement = important_snvs[i].annotations().at(snv_index_cgi_driver_statement_);

		if(is_cgi_transcript && variant_types.count() == 1 )
		{
			if(driver_statement.contains("known"))
			{
				QByteArrayList temp_cancer_acronyms = driver_statement.split(':')[1].split(';');

				for(int j=0;j<temp_cancer_acronyms.count();j++)
				{
					if(!cancer_acronyms.contains(temp_cancer_acronyms[j]))
					{
						cancer_acronyms.append(temp_cancer_acronyms[j].trimmed());
					}
				}
				if(display_germline_hint)
				{
					driver_statement.replace("known","known driver");
					driver_statement.replace(";",", ");
				}
				else
				{
					driver_statement = "known driver";
				}
			}
			if(driver_statement.contains("predicted"))
			{
				driver_statement.replace(": tier 1","");
				driver_statement.replace(": tier 2","");
			}
			columns.append("\\li20 " + driver_statement);
		}
		else
		{
			columns.append("\\li20 NA");
		}


		//gene role
		//oncogene: tumor driver and Activating
		if(is_cgi_transcript && variant_types.count() == 1)
		{
			bool is_driver = important_snvs[i].annotations().at(snv_index_cgi_driver_statement_).contains("driver") ||
					important_snvs[i].annotations().at(snv_index_cgi_driver_statement_).contains("known");

			if(is_driver && important_snvs[i].annotations().at(snv_index_cgi_gene_role_) == "Act")
			{
				columns.append("\\qc Activating");
			}
			else if(is_driver && important_snvs[i].annotations().at(snv_index_cgi_gene_role_) == "LoF")
			{
				columns.append("\\qc Inactivating");
			}
			else if(is_driver && important_snvs[i].annotations().at(snv_index_cgi_gene_role_) ==("ambiguous"))
			{
				columns.append("\\qc Ambiguous");
			}
			else
			{
				columns.append("\\qc NA");
			}
		}
		else
		{
			columns.append("\\qc NA");
		}

		somatic_snv_table_unordered.append(columns);
	}

	//reorder SNV table: add drivers
	QList< QList<QString> > somatic_snv_table;
	foreach(QList<QString> row, somatic_snv_table_unordered)
	{
		bool is_driver = false;

		for(int i=0;i<row.count();i++)
		{
			if(row[i].contains("driver") || row[i].contains("known")) is_driver = true;
		}

		if(is_driver)
		{
			somatic_snv_table.append(row);
		}
	}
	//reorder SNV table: add passengers
	foreach(QList<QString> row,somatic_snv_table_unordered)
	{
		bool is_driver =false;
		for(int i=0;i<row.count();i++)
		{
			if(row[i].contains("driver") || row[i].contains("known")) is_driver = true;
		}

		if(!is_driver && !row[4].contains("NA"))
		{
			somatic_snv_table.append(row);
		}
	}
	//reorder SNV table: add "NA"
	foreach(QList<QString> row,somatic_snv_table_unordered)
	{
		if(row[4].contains("NA"))
		{
			somatic_snv_table.append(row);
		}
	}


	widths.clear();
	widths = colWidths;
	RtfTools::writeRtfWholeTable(stream,somatic_snv_table,widths,18,true,false);

	if(display_germline_hint)
	{
		widths.clear();
		widths << max_table_width;
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "\\sb20\\qj\\fs18\\qj\\b " << "Keimbahnvarianten:\\b0 ";
		stream << "{\\highlight3  Siehe Zusatzbefund / Es wurden {\\b\\ul keine} pathogenen Keimbahnvarianten in den untersuchten Genen ";
		for(int i=0;i<genes_checked_for_germline_variants_.count();i++)
		{
			stream << genes_checked_for_germline_variants_[i];
			if(i<genes_checked_for_germline_variants_.count()-1)
			{
				stream << ", ";
			}
		}
		stream << " gefunden}" << endl;

		stream << "\\cell\\row}" << endl;
	}

	if(display_germline_hint)
	{
		widths.clear();
		widths << max_table_width;
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "\\fs18\\sb20 "<<endl;
		stream <<"\\qj\\fs16\\qj\\b Abk\\u252;rzungen:\\b0  " << "{\\i cDNA:} cDNA Position und Auswirkung auf Peptid; {\\i F/T Tumor:} Allelfrequenz/Tiefe der Tumorprobe; ";
		stream << "{\\i Funktion:} Funktionelle Einsch\\u228;tzung der Variante aus CancerGenomeInterpreter.org. \"Known driver\" bedeutet, dass die Ver\\u228;nderung in bestimmten Tumortypen bekannt ist. \"Predicted driver\" bedeutet, dass die Variante mittels probabilistischer Methoden als Tumortreiber vorhergesagt wurde. \"NA\" bedeutet, dass keine Einsch\\u228;ng vorliegt. ";
		stream << "{\\i Effekt:} Auswirkung der Mutation auf das Gen. Activating: aktivierende Wirkung auf ein Onkogen, Inactivating: Funktionsverlust eines Tumorsuppressorgens, Ambiguous: zweideutige Wirkung auf das Gen, NA: keine Einsch\\u228;tzung verf\\u252;gbar.";
		stream << "\\line {\\i Akronyme:} ";

		QHash<QByteArray,QByteArray> acronyms_to_german;

		TSVFileStream acronym_translations("://Resources/cancer_types.tsv");
		int i_cgi_acronym = acronym_translations.colIndex("ID",true);
		int i_german_translation = acronym_translations.colIndex("NAME_GERMAN",true);
		while(!acronym_translations.atEnd())
		{
			QByteArrayList current_line = acronym_translations.readLine();
			acronyms_to_german.insert(current_line.at(i_cgi_acronym),current_line.at(i_german_translation));
		}

		std::sort(cancer_acronyms.begin(),cancer_acronyms.end());
		for(int i=0;i<cancer_acronyms.count();i++)
		{
			stream << cancer_acronyms[i] << " - " << acronyms_to_german.value(cancer_acronyms[i]);
			if(i<cancer_acronyms.count()-1)
			{
				stream << ", ";
			}
		}
		stream << "\\cell" << "\\row}" << endl;
	}
}

void ReportHelper::writeRtfTableCNV(QTextStream& stream, const QList<int>& colWidths)
{
	CnvList important_cnvs = filterCnv();

	int max_table_width = colWidths.last();

	//beginning of rtf cells
	QString begin_table_cell = "\\pard\\intbl\\fs18 ";

	QList<int> widths;

	//widths if cnv_type is set in input VariantLists
	QList<int> widths_cnv_type;
	widths_cnv_type << 4500 << 5000 << 5400 << 7700 << 8700 << max_table_width;
	//CNVs
	widths << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs18\\b Kopienzahlvarianten (CNVs)\\b0\\par}" << endl;


	widths.clear();
	widths = colWidths;

	QList< QList<QString> > header_cnvs;
	QList<QString> header_columns;

	header_columns <<"\\trhdr\\qc Gene" << "\\qc CNV" << "\\qc CN" << "\\qc Position" << "\\qc Gr\\u246;\\u223;e [kb]";

	if(cnv_index_cnv_type_!=-1)
	{
		header_columns << "\\qc Typ";
	}

	header_cnvs << header_columns;
	if(cnv_index_cnv_type_!=-1)
	{
		RtfTools::writeRtfWholeTable(stream,header_cnvs,widths_cnv_type,18,true,true);
	}
	else
	{
		RtfTools::writeRtfWholeTable(stream,header_cnvs,widths,18,true,true);
	}

	//construct 2D QStringList to create RTF table
	QList< QList<QString> > somatic_cnv_table;
	//neccessary for filtering for target region
	//create set with target genes
	QString target_region = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true).target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region.left(target_region.size()-4) + "_genes.txt");
	target_genes = db_.genesToApproved(target_genes);

	for(int i=0;i<important_cnvs.count();i++)
	{
		CopyNumberVariant variant = important_cnvs[i];
		QList<QString> columns;

		//gene names
		//only print genes which which lie in target region
		if(!variant.annotations().at(cnv_index_cgi_genes_).isEmpty())
		{
			//cgi genes
			GeneSet cgi_genes = GeneSet::createFromText(variant.annotations().at(cnv_index_cgi_genes_),',');
			cgi_genes = db_.genesToApproved(cgi_genes);

			QByteArrayList cgi_driver_statements = variant.annotations().at(cnv_index_cgi_driver_statement_).split(',');

			QString genes_included = "";

			for(int j=0;j<cgi_genes.count();j++)
			{
				//skip if gene is not contained in target region
				if(!target_genes.contains(cgi_genes[j]) && !cnv_keep_genes_filter_.contains(cgi_genes[j]))
				{
					continue;
				}

				//driver gene is printed bold
				if(cgi_driver_statements[j].contains("driver") || cgi_driver_statements[j].contains("known"))
				{
					genes_included.append("\\b " + cgi_genes[j] +"\\b0 ");
				}
				else
				{
					genes_included.append(cgi_genes[j]);
				}

				if(j < cgi_genes.count()-1)
				{
					genes_included.append(", ");
				}
			}
			columns.append("\\li20 " + genes_included);
		}
		else
		{
			columns.append("");
		}


		//AMP/DEL
		QVector<double> copy_numbers;
		foreach(int cn, variant.copyNumbers())
		{
			copy_numbers.append((double)cn);
		}
		std::sort(copy_numbers.begin(),copy_numbers.end());
		//median Copy Number
		double cn = BasicStatistics::median(copy_numbers);
		if(cn > 2.)
		{
			columns.append("\\qc AMP");
		}
		else
		{
			columns.append("\\qc DEL");
		}


		//copy numbers
		columns.append("\\qc " + QString::number(cn));


		//coordinates
		columns.append("\\fi20 " +variant.chr().str() + ":" + QString::number(variant.start()) + "-" + QString::number(variant.end()));


		//size
		columns.append("\\qr " + QString::number(round((important_cnvs[i].end()-important_cnvs[i].start())/1000.)) + "\\ri20");

		//
		if(cnv_index_cnv_type_!=-1)
		{
			columns.append("\\qc " + variant.annotations().at(cnv_index_cnv_type_));
		}

		somatic_cnv_table.append(columns);
	}
	widths.clear();
	if(cnv_index_cnv_type_!=-1)
	{
		widths = widths_cnv_type;
	}
	else
	{
		widths = colWidths;
	}
	RtfTools::writeRtfWholeTable(stream,somatic_cnv_table,widths,18,true,false);


	//Germline
	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\sb20\\qj\\fs18\\qj\\b " << "Keimbahnvarianten:\\b0 ";
	stream << "{\\highlight3  Siehe Zusatzbefund / Es wurden keine pathogenen Keimbahnvarianten gefunden.}" << endl;
	stream << "\\cell\\row}" << endl;

	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);

	stream << begin_table_cell << "\\sb20\\fs16\\qj\\b Abk\\u252;rzungen:\\b0  " << "Gene: Gene aus der Zielregion. Tumortreiber sind fett gedruckt und Passenger normal; CNV: Amplifikation (AMP) oder Deletion (DEL); CN: Copy Number; Position: Chromosomale Position (GRCh37); Gr\\u0246;\\u0223;e: Ausdehnung der CNV in Kilobasen [kb]." ;
	stream << "\\cell\\row}" << endl;

	stream << "{\\par\\pard}" << endl;


	//Summary Amplified/Deleted genes, include passengers and drivers
	GeneSet amplified_cnvs;
	GeneSet deleted_cnvs;

	for(int i=0;i<important_cnvs.count();i++)
	{
		CopyNumberVariant variant = important_cnvs[i];

		QVector<double> copy_numbers;
		foreach(int cn, variant.copyNumbers())
		{
			copy_numbers.append((double)cn);
		}
		std::sort(copy_numbers.begin(),copy_numbers.end());
		//median Copy Number
		double cn = BasicStatistics::median(copy_numbers);

		if(cn > 2.)
		{
			amplified_cnvs.insert(GeneSet::createFromText(variant.annotations().at(cnv_index_cgi_genes_),','));
		}
		else
		{
			deleted_cnvs.insert(GeneSet::createFromText(variant.annotations().at(cnv_index_cgi_genes_),','));
		}

	}

	widths.clear();
	widths << 2000 << max_table_width;
	qSort(deleted_cnvs.begin(),deleted_cnvs.end());
	qSort(amplified_cnvs.begin(),amplified_cnvs.end());
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\ql\\highlight3 " << "Amplifizierte Gene:" << "\\cell"  <<endl;
	stream << begin_table_cell << "\\ql\\highlight3 " << endl;
	stream << "\\qj ";

	for(int i=0;i<amplified_cnvs.count();i++)
	{
		if(!target_genes.contains(amplified_cnvs[i])) continue;

		stream << amplified_cnvs[i];

		if(i<amplified_cnvs.count()-1 && target_genes.contains(amplified_cnvs[i+1]))
		{
			stream << ", ";
		}
	}
	stream << "\\cell" << "\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\ql\\highlight3 " << "Deletierte Gene:" << "\\cell"  <<endl;
	stream << "\\qj\\hightlight3 ";

	for(int i=0;i<deleted_cnvs.count();i++)
	{
		if(!target_genes.contains(deleted_cnvs[i])) continue;

		stream << deleted_cnvs[i];
		if(i<deleted_cnvs.count()-1 )
		{
			stream << ", ";
		}
	}
	stream << "\\cell" << "\\row}" << endl;
}

CGIDrugReportLine::CGIDrugReportLine()
{
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


	//group drugs into different groups

	QList< QList<QString> > all_drugs;

	while(!file.atEnd())
	{
		QList<QByteArray> line_tmp = file.readLine();
		QList<QString> line;
		for(int i=0;i<line_tmp.count();i++)
		{
			line.append(line_tmp.at(i));
		}

		all_drugs.append(line);
	}

	//parse text in "source", replace denominators
	for(int i=0;i<all_drugs.count();i++)
	{
		all_drugs[i][i_source] = all_drugs[i][i_source].replace(";",", ");
	}

	//Group drugs by evidence level:
	//1: same tumor and evidence according guidlines
	//2: other tumor and evidence according guidelines of other tumor entity
	//3: same tumor and evidence according Late trials
	//4: other tumor and evidence according Late trials of other tumor entity
	//5: same tumor and evidence according Pre-clinical, Early Trials or Case report
	foreach(QList<QString> line, all_drugs)
	{
		//skip all entries which differ in mutation
		if(line.at(i_alteration_match) != "complete") continue;

		CGIDrugReportLine row;

		if(!line.at(i_sample_alteration).contains("amp") && !line.at(i_sample_alteration).contains("del"))
		{
			row.gene_ = line.at(i_sample_alteration).split(' ')[0];
			row.alteration_type_ = line.at(i_sample_alteration);//.split(' ')[1];
		}
		else
		{
			row.gene_ = line.at(i_sample_alteration).split(':')[0];
			row.alteration_type_ = line.at(i_sample_alteration);//.split(' ')[0].split(':')[1].toUpper();
		}
		row.drug_ = line.at(i_drug);
		row.effect_ = line.at(i_effect);
		row.evidence_ = line.at(i_evidence);
		row.entity_ = line.at(i_tumor_entity);
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
		drug_list_.insert(evidence_level,row);
	}
}

void CGIDrugTable::removeDuplicateDrugs()
{
	foreach(CGIDrugReportLine line_1,drug_list_.values(1))
	{
		for(int i=2;i<=5;i++) //remove starting at evidence level 2
		{
			foreach(CGIDrugReportLine line_2,drug_list_.values(i))
			{
				if(line_1 == line_2) drug_list_.remove(i,line_2);
			}
		}
	}
}

const QList<QList<QString>> CGIDrugTable::drugsByEvidAsString(int evid_group)
{
	QList<CGIDrugReportLine> temp_results = drugsByEvidenceLevel(evid_group);
	QList<QList<QString>> results;

	foreach(CGIDrugReportLine line,temp_results)
	{
		QList<QString> result_line = line.asStringList();
		for(int i=0;i<result_line.count();i++)
		{
			result_line[i].prepend("\\li20 ");

			//format alteration type

			if(result_line[i].contains("MUT"))
			{
				result_line[i].replace(" MUT (",":");
				result_line[i].replace(")","");
			}
			if(result_line[i].contains(":del"))
			{
				result_line[i].replace(" CNA","");
				result_line[i].replace("del","DEL");
			}
			if(result_line[i].contains(":amp"))
			{
				result_line[i].replace(" CNA","");
				result_line[i].replace("amp","AMP");
			}
		}
		result_line.removeFirst();
		results.append(result_line);
	}
	return results;
}

const QList<QByteArray> CGIDrugTable::getAcronyms(int evid_level) const
{
	QList<CGIDrugReportLine> drugs = drugsByEvidenceLevel(evid_level);

	QList<QByteArray> results;

	foreach(CGIDrugReportLine drug,drugs)
	{
		QByteArray temp_tumor_entities = drug.entity().toUtf8();
		temp_tumor_entities.replace(" ","");
		QList<QByteArray> tumor_entities = temp_tumor_entities.split(',');

		for(int i=0;i<tumor_entities.count();i++)
		{
			results.append(tumor_entities[i]);
		}
	}

	return results;
}

ReportHelper::ReportHelper()
{
}

ReportHelper::ReportHelper(QString snv_filename, GeneSet cnv_keep_genes_filter, QString target_region)
	: snv_filename_(snv_filename)
	, db_()
	, target_region_(target_region)
{
	snv_variants_.load(snv_filename_);

	QString base_name = QFileInfo(snv_filename_).baseName();
	QFileInfo(snv_filename_).dir().cdUp();

	//filename for CNV file
	cnv_filename_ = base_name + "_cnvs.tsv";
	cgi_drugs_path_ = QFileInfo(snv_filename_).absolutePath() + "/" + base_name + "_cgi_drug_prescription.tsv";

	cnv_variants_.load(QFileInfo(snv_filename_).absolutePath().append("/").append(cnv_filename_));

	//extract obo terms for filtering
	OntologyTermCollection obo_terms("://Resources/so-xp_3_0_0.obo",true);
	QList<QByteArray> ids;
	ids << obo_terms.childIDs("SO:0001580",true); //coding variants
	ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
	foreach(const QByteArray& id, ids)
	{
		obo_terms_coding_splicing_.add(obo_terms.findByID(id));
	}

	//load genes that lie in target region
	genes_in_target_region_ = GeneSet::createFromFile(target_region_.left(target_region_.size()-4)+"_genes.txt");

	//get cancer type which was used for CGI analysis and text for ICD10-diagnosis
	for(int i=0;i<snv_variants_.comments().count();i++)
	{
		if(snv_variants_.comments().at(i).contains("CGI_ICD10_TEXT"))
		{
			icd10_diagnosis_text_ = snv_variants_.comments().at(i).split('=')[1];
		}
		if(snv_variants_.comments().at(i).contains("CGI_CANCER_TYPE"))
		{
			cgi_cancer_type_ = snv_variants_.comments().at(i).split('=')[1];
		}
	}

	if(cgi_cancer_type_.isEmpty())
	{
		cgi_cancer_type_ = "empty";
	}

	if(icd10_diagnosis_text_.isEmpty())
	{
		icd10_diagnosis_text_ = "empty";
	}


	cnv_keep_genes_filter_ = cnv_keep_genes_filter;

	//assign columns indices for SNV file
	snv_index_filter_ = snv_variants_.annotationIndexByName("filter",true,true);
	snv_index_coding_splicing_ = snv_variants_.annotationIndexByName("coding_and_splicing",true,true);

	snv_index_cgi_driver_statement_ = snv_variants_.annotationIndexByName("CGI_driver_statement",true,true);
	snv_index_cgi_gene_role_ = snv_variants_.annotationIndexByName("CGI_gene_role",true,true);
	snv_index_cgi_transcript_ = snv_variants_.annotationIndexByName("CGI_transcript",true,true);
	snv_index_cgi_gene_ = snv_variants_.annotationIndexByName("CGI_gene",true,true);


	cnv_index_cgi_gene_role_ = -1;
	cnv_index_cnv_type_ = -1;
	cnv_index_cgi_genes_ = -1;
	cnv_index_cgi_driver_statement_ = -1;

	for(int i=0;i<cnv_variants_.annotationHeaders().count();i++)
	{
		if(cnv_variants_.annotationHeaders()[i] == "CGI_gene_role")
		{
			cnv_index_cgi_gene_role_ = i;
		}

		if(cnv_variants_.annotationHeaders()[i] == "cnv_type")
		{
			cnv_index_cnv_type_ = i;
		}

		if(cnv_variants_.annotationHeaders()[i] == "CGI_genes")
		{
			cnv_index_cgi_genes_ = i;
		}

		if(cnv_variants_.annotationHeaders()[i] == "CGI_driver_statement")
		{
			cnv_index_cgi_driver_statement_ = i;
		}
	}

	if(cnv_index_cgi_driver_statement_ == -1)
	{
		THROW(FileParseException,"Could not create RTF report: CNV file does not contain CGI_driver_statement column");
	}
	else if(cnv_index_cgi_gene_role_ == -1 )
	{
		THROW(FileParseException,"Could not create RTF report: CNV file does not contain CGI_gene_role column");
	}
	else if(cnv_index_cgi_genes_ == -1)
	{
		THROW(FileParseException,"Could not create RTF report: CNV file does not contain CGI_genes column");
	}


	tumor_id_ = base_name.split('-')[0];
	normal_id_ = base_name.split('-')[1].split('_')[0]+'_'+base_name.split('-')[1].split('_')[1];

	//load qcml data
	QString qcml_file = base_name;
	qcml_file.append("_stats_som.qcML");
	QString qcml_file_absolute_path = QFileInfo(snv_filename).absolutePath().append("/").append(qcml_file);
	qcml_data_ = QCCollection::fromQCML(qcml_file_absolute_path);
	roi_.load(target_region);


	//load metadata from GenLab
	//get histological tumor proportion and diagnosis from GenLab
	GenLabDB db_genlab;
	icd10_diagnosis_code_ = db_genlab.diagnosis(tumor_id_.split('_')[0]);
	histol_tumor_fraction_ = db_genlab.tumorFraction(tumor_id_.split('_')[0]);

	mutation_burden_ = qcml_data_.value("QC:2000053",true).asString().split(' ')[1].remove('(').toDouble();

	genes_checked_for_germline_variants_ = GeneSet::createFromFile(db_.getTargetFilePath(true,true) + "ACMG_25Onkogene_20_genes.txt");
}

VariantList ReportHelper::filterSnvForCGIAnnotation(bool filter_for_target_region)
{
	//save variants which are included in SNV report in this VariantList
	VariantList important_snvs;
	important_snvs.copyMetaData(snv_variants_);

	//Skip unimportant SNVs: CGI, filtered, OBO type
	for(int i=0;i<snv_variants_.count();i++)
	{
		Variant variant = snv_variants_[i];
		QByteArray cgi_gene = variant.annotations().at(snv_index_cgi_gene_);

		//skip variants which do not lie in target region
		if(filter_for_target_region && !genes_in_target_region_.contains(cgi_gene)) continue;
		//Skip variants that are filtered in variant file (the ones which have an entry in the filtering column except freq-tum)
		if(!variant.annotations().at(snv_index_filter_).isEmpty() && variant.annotations().at(snv_index_filter_) != "freq-tum") continue;
		//Skip variants which have no CGI annotation
		if(variant.annotations().at(snv_index_cgi_driver_statement_).isEmpty()) continue;

		//Skip variant types which are non-coding or non-splicing by comparison with MISO terms
		QByteArrayList tmp_variant_types = variant.annotations().at(snv_index_coding_splicing_).split(',');
		bool skip = true;
		for(int j=0;j<tmp_variant_types.length();j++)
		{
			QByteArrayList tmp_single_variant_types =tmp_variant_types[j].split(':')[2].split('&');
			for(int k=0;k<tmp_single_variant_types.length();k++)
			{
				//check whether variant type is in OBO terms
				if(obo_terms_coding_splicing_.containsByName(tmp_single_variant_types[k]) || obo_terms_coding_splicing_.containsByName(tmp_single_variant_types[k]+"_variant"))
				{
					skip = false;
				}
			}
		}
		if(skip) continue;

		important_snvs.append(variant);
	}

	return important_snvs;
}

CnvList ReportHelper::filterCnv()
{
	//cgi annotated cnvs
	CnvList cgi_annotated_cnvs;

	cgi_annotated_cnvs.copyMetaData(cnv_variants_);

	for(int i=0;i<cnv_variants_.count();i++)
	{
		bool skip = false;
		CopyNumberVariant variant = cnv_variants_[i];

		//Prepare array with z-scores
		QList<double> zscores = variant.zScores();
		for(int j=0;j<zscores.count();j++)
		{
			zscores[j] = fabs(zscores[j]);
		}
		//only keep genes with high enough z-scores
		if(*std::max_element(zscores.begin(),zscores.end()) < 5.)
		{
			skip = true;
		}
		if(zscores.length()< 10.)
		{
			skip = true;
		}

		foreach (QByteArray gene, variant.genes())
		{

			if(cnv_keep_genes_filter_.contains(gene) && fabs(*std::max_element(zscores.begin(),zscores.end()))>=5.)
			{
				skip = false;
			}
		}

		if(skip) continue;

		cgi_annotated_cnvs.append(variant);

	}

	return cgi_annotated_cnvs;
}

QHash<QByteArray, BedFile> ReportHelper::gapStatistics()
{
	BedFile roi_inter;
	QString processed_sample_id = db_.processedSampleId(tumor_id_);
	ProcessingSystemData system_data = db_.getProcessingSystemData(processed_sample_id, true);
	roi_inter.load(system_data.target_file);
	roi_inter.intersect(roi_);
	roi_inter.merge();

	//Import BedFile with low coverage statistics
	QString low_cov_file = snv_filename_;
	low_cov_file.replace(".GSvar", "_stat_lowcov.bed");
	BedFile low_cov;
	low_cov.load(low_cov_file);
	low_cov.intersect(roi_);

	if (roi_inter.baseCount()!=roi_.baseCount())
	{
		QString message = "Gaps cannot be calculated because the selected target region is larger than the processing system target region:";
		BedFile roi_missing;

		roi_missing = roi_;
		roi_missing.merge();
		roi_missing.subtract(roi_inter);
		for (int i=0; i<std::min(10, roi_missing.count()); ++i)
		{
			message += "\n" + roi_missing[i].toString(true);
		}
		//TODO: NULL pointer instead of parent?
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
	QHash<QByteArray, BedFile> grouped;
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

void ReportHelper::writeRtfCGIDrugTable(QTextStream &stream, const QList<int> &col_widths)
{
	int max_table_width = col_widths.last();
	QString begin_table_cell = "\\pard\\intbl\\fs18\\fi20 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b\\fi20 ";

	QList<int> widths;
	QList<int> widths_drug_table;
	//				     alt.typ  tum.typ medic.  eff.   evid.    source
	widths_drug_table << 1500 << 2700 << 5000 << 6000 << 7000 << max_table_width;

	widths = widths_drug_table;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << "\\trhdr ";
	stream << begin_table_cell_bold << "\\fi20 Gen\\cell";
	stream << begin_table_cell_bold << "\\fi20 Tumortyp\\cell" << begin_table_cell_bold << "\\fi20 Medikament\\cell";
	stream << begin_table_cell_bold << "\\fi20 Effekt\\cell" << begin_table_cell_bold << "\\fi20 Evidenz\\cell";
	stream << begin_table_cell_bold << "\\fi20 Quelle\\cell" << "\\row}" << endl;

	CGIDrugTable drugs;
	drugs.load(cgi_drugs_path_);
	drugs.removeDuplicateDrugs();


	RtfTools::writeRtfWholeTable(stream,drugs.drugsByEvidAsString(1),widths,18,true,false);
	RtfTools::writeRtfWholeTable(stream,drugs.drugsByEvidAsString(2),widths,18,true,false);
	RtfTools::writeRtfWholeTable(stream,drugs.drugsByEvidAsString(3),widths,18,true,false);
	RtfTools::writeRtfWholeTable(stream,drugs.drugsByEvidAsString(4),widths,18,true,false);
	RtfTools::writeRtfWholeTable(stream,drugs.drugsByEvidAsString(5),widths,18,true,false);

	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18\\sb20"<<endl;
	stream << "\\qj\\fs16\\qj\\b Abk\\u252;rzungen:\\b0  " << "Gen: Betroffenes Gen und Ver\\u228;nderung; ";
	stream << "Tumortyp: Tumor, auf den sich das Medikament bezieht.\\line" << endl;
	stream << "Bedeutung der gelisteten Akronyme: ";

	QHash<QByteArray,QByteArray> acronyms_to_german;

	TSVFileStream acronym_translations("://Resources/cancer_types.tsv");
	int i_cgi_acronym = acronym_translations.colIndex("ID",true);
	int i_german_translation = acronym_translations.colIndex("NAME_GERMAN",true);
	while(!acronym_translations.atEnd())
	{
		QByteArrayList current_line = acronym_translations.readLine();
		acronyms_to_german.insert(current_line.at(i_cgi_acronym),current_line.at(i_german_translation));
	}
	QSet<QByteArray> temp_acronyms = drugs.getAcronyms(1).toSet();
	temp_acronyms.unite(drugs.getAcronyms(2).toSet());
	temp_acronyms.unite(drugs.getAcronyms(3).toSet());
	temp_acronyms.unite(drugs.getAcronyms(4).toSet());
	temp_acronyms.unite(drugs.getAcronyms(5).toSet());

	QList<QByteArray> acronyms = temp_acronyms.toList();
	qSort(acronyms);
	for(int i=0;i<acronyms.count();i++)
	{
		stream << acronyms[i] << "-" << acronyms_to_german.value(acronyms[i]);
		if(i<acronyms.count()-1)
		{
			stream << ", ";
		}
	}

	stream << "\\cell\\row}" << endl;
}

void ReportHelper::germlineSnvForQbic()
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

void ReportHelper::somaticSnvForQbic()
{
	QString path_target_folder = QFileInfo(snv_filename_).absolutePath() + "/" + "QBIC_files/";

	if(!QDir(path_target_folder).exists())
	{
		QDir().mkdir(path_target_folder);
	}

	snv_variants_ = gsvarToVcf();

	QSharedPointer<QFile> somatic_snvs_qbic = Helper::openFileForWriting(path_target_folder+"QBIC_somatic_snv.tsv");

	QTextStream stream(somatic_snvs_qbic.data());

	//Write header
	stream << "chr" <<"\t" << "start" << "\t" << "ref" << "\t" << "alt" << "\t";
	stream <<"allele_frequency_tumor" << "\t" << "coverage" << "\t";
	stream << "gene" << "\t" << "base_change" << "\t" << "aa_change" << "\t";
	stream << "transcript" << "\t" << "functional_class" << "\t" << "effect" << endl;

	VariantList variants = filterSnvForCGIAnnotation();

	int i_tumor_af = variants.annotationIndexByName("tumor_af",true,true);
	int i_tumor_depth = variants.annotationIndexByName("tumor_dp",true,true);

	for(int i=0;i<variants.count();i++)
	{
		Variant variant = variants[i];

		stream << variant.chr().str() << "\t";
		stream << variant.start() << "\t";
		stream << variant.ref() << "\t";
		stream << variant.obs()<< "\t";
		stream << variant.annotations().at(i_tumor_af) << "\t";
		stream << variant.annotations().at(i_tumor_depth) << "\t";

		//determine transcript, if available take CGI transcript. Take first in list otherwise
		QByteArrayList transcripts = variant.annotations().at(snv_index_coding_splicing_).split(',');
		QByteArray cgi_transcript = variant.annotations().at(snv_index_cgi_transcript_);
		int take_transcript = 0;
		bool cgi_transcript_exits = false;
		if(!cgi_transcript.isEmpty())
		{
			for(int j=0;j<transcripts.count();j++)
			{

				QByteArray transcript_ens_id = transcripts[j].split(':')[1];
				if(transcript_ens_id == cgi_transcript )
				{
					take_transcript = j;
					cgi_transcript_exits =true;
					break;
				}
			}
		}

		//sometimes CGI reports intronic effects => check whether the functional class of  chosen transcript is coding or splicing
		bool is_coding_splicing = false;
		QByteArrayList functional_classes = transcripts[take_transcript].split(':')[2].split('&');
		foreach(QByteArray sub_class,functional_classes)
		{
			if(obo_terms_coding_splicing_.containsByName(sub_class))
			{
				is_coding_splicing = true;
				break;
			}
		}

		//if not coding/splicing take first transcript that is coding and splicing
		if(!is_coding_splicing)
		{
			for(int j=0;j<transcripts.count();j++)
			{
				bool take_this_transcript = false;
				QByteArrayList effects = transcripts[j].split(':')[2].split('&');
				foreach(QByteArray sub_effect,effects)
				{
					if(obo_terms_coding_splicing_.containsByName(sub_effect))
					{
						take_this_transcript = true;
						break;
					}
				}
				if(take_this_transcript)
				{
					take_transcript = j;
					break;
				}
			}
		}

		QByteArrayList transcripts_data = transcripts[take_transcript].split(':');

		//affected gene
		stream << transcripts_data[0] << "\t";

		//base change
		stream << transcripts_data[5] << "\t";
		//protein change
		stream << transcripts_data[6] << "\t";
		//transcript
		stream << transcripts_data[1] << "\t";
		//functional class
		stream << transcripts_data[2].replace('&',',') << "\t";

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

		//if CGI transcript does not agree with our prediction that the SNV is coding/splicing, set CGI effect to NA
		if(!is_coding_splicing || !cgi_transcript_exits)
		{
			effect = "NA";
		}

		stream << effect;

		stream << endl;
	}
	somatic_snvs_qbic->close();
}

void ReportHelper::germlineCnvForQbic()
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


void ReportHelper::somaticCnvForQbic()
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

	CnvList variants = filterCnv();

	QString target_region_processing_system = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true).target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region_processing_system.left(target_region_processing_system.size()-4) + "_genes.txt");
	NGSD db;
	target_genes = db.genesToApproved(target_genes);

	for(int i=0;i<variants.count();i++)
	{
		CopyNumberVariant variant = variants[i];

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

		//Deletion or Amplification
		QVector<double> copy_numbers;
		for(int i=0;i<variant.copyNumbers().count();i++)
		{
			copy_numbers.append((double)variant.copyNumbers()[i]);
		}

		std::sort(copy_numbers.begin(),copy_numbers.end());
		int copy_number = BasicStatistics::median(copy_numbers,true);
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
			for(int j=0;j<genes_in_report.count();j++)
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
		for(int j=0;j<effects.count();j++)
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


		for(int j=0;j<effects.count();j++)
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

			stream << genes[j] << ":" << effect;

			if(j<effects.count()-1 && is_driver[j+1] ) stream << ";";
		}

		stream << endl;
	}

	somatic_cnvs_qbic->close();

}

void ReportHelper::somaticSvForQbic()
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

void ReportHelper::metaDataForQbic()
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

VariantList ReportHelper::gsvarToVcf()
{
	//load original VCF
	QString orig_name = snv_filename_;
	orig_name.replace(".GSvar", "_var_annotated.vcf.gz");

	if (!QFile::exists(orig_name))
	{
		QString message = "Could not find file " + orig_name +". Files for QBIC were not created";
		QMessageBox::warning(NULL, "Original vcf data", message);
	}
	VariantList orig_vcf;
	orig_vcf.load(orig_name, VariantList::VCF_GZ);

	if(orig_vcf.count() != snv_variants_.count())
	{
		QString message = "Could not change coordinates in VariantList because line numbers in GSVar file and vcf file are different.";
		QMessageBox::warning(NULL,"Failed changing coordinates in VariantList",message);
		return snv_variants_;
	}

	VariantList output = snv_variants_;

	for(int i=0;i<snv_variants_.count();i++)
	{
		Variant gsvar_variant = snv_variants_[i];
		Variant vcf_variant = orig_vcf[i];

		//transform del-coordinates: pos is +1 in GSVar,
		if(gsvar_variant.obs() == "-")
		{
			//check positions
			if(gsvar_variant.start()-1 == vcf_variant.start())
			{
				output[i].setObs(vcf_variant.obs());
				output[i].setRef(vcf_variant.ref());
				output[i].setStart(vcf_variant.start());
			}
			else
			{
				QString message = "Could not transform coordinates in GSVar-file to original vcf coordinates because positions do not match.";
				QMessageBox::warning(NULL,"Failed changing coordinates in VariantList",message);
				return snv_variants_;
			}
		}

		//transform insertions: ref is - in GSVar
		if(gsvar_variant.ref() == "-")
		{
			//check positions (stay the same)
			if(gsvar_variant.start() == vcf_variant.start())
			{
				output[i].setObs(vcf_variant.obs());
				output[i].setRef(vcf_variant.ref());
			}
			else
			{
				QString message = "Could not transform coordinates in GSVar-file to original vcf coordinates because positions do not match.";
				QMessageBox::warning(NULL,"Failed changing coordinates in VariantList",message);
				return snv_variants_;
			}
		}

	}
	return output;
}

void ReportHelper::writeRtf(const QString& out_file)
{

	QString begin_table_cell = "\\pard\\intbl\\fs18 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";

	//max table widths in rtf file
	int max_table_width = 9921;
	QList<int> widths;
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(out_file);
	QTextStream stream(outfile.data());

	RtfTools::writeRtfHeader(stream);

	//create SNV table with driver mutations only
	//        Gene    cDNA    type    F/T     func	   eff
	widths << 1250 << 4000 << 5400 << 6500 << 8950  << max_table_width;

	writeRtfTableSNV(stream,widths,false);

	stream << endl <<  "\\line" << endl;

	widths.clear();
	widths << 3000 << max_table_width;

	stream << "{\\pard\\b Anhang zum Befund vom {\\highlight3 " << QDate::currentDate().toString("dd.MM.yyyy") << "}\\par}" << endl;
	stream << "{\\pard\\par}" << endl;
	stream << "{\\pard\\sa45\\fs18\\b Allgemeine Informationen:\\par}" << endl;

	double tumor_molecular_proportion = qcml_data_.value("QC:2000054",true).toString().toDouble();

	if(tumor_molecular_proportion > 100.)
	{
		tumor_molecular_proportion = 100.;
	}

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Datum:\\cell" << begin_table_cell <<QDate::currentDate().toString("dd.MM.yyyy") << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Proben-ID (Tumor):\\cell" << begin_table_cell << tumor_id_ << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Proben-ID (Keimbahn):\\cell" << begin_table_cell << normal_id_ << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Tumoranteil histol./molekular:\\cell" << begin_table_cell << histol_tumor_fraction_ << "\%";
	stream << " / "<< tumor_molecular_proportion <<"\%\\cell\\row}" << endl;
	//rtf somatic mutation rate, only if mutation_burden was passed as parameter
	if(mutation_burden_>=0.)
	{
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "Mutationslast:" << "\\cell" << begin_table_cell << mutation_burden_ << " Varianten/Mbp";
		QString mutation_burden_assesment;
		if(mutation_burden_<3.3) mutation_burden_assesment = "niedrig";
		if(mutation_burden_<=23.1 && mutation_burden_>=3.3) mutation_burden_assesment = "mittel";
		if(mutation_burden_>23.1) mutation_burden_assesment = "hoch";
		stream << " (" << mutation_burden_assesment << ") " << endl;
		stream << "\\cell\\row}" << endl;
	}


	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Diagnose:\\cell" << begin_table_cell << icd10_diagnosis_text_ << " (ICD10: "<< icd10_diagnosis_code_ << ")" << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "CGI-Tumortyp:\\cell" << begin_table_cell << cgi_cancer_type_ << "\\cell\\row}" << endl;

	stream << "\\line" << endl;

	//SNV report with details
	stream << "{\\pard\\sb45\\sa45\\fs18\\b Einzelnukleotid-Varianten (SNV) und kleine Insertionen/Deletionen (INDEL)\\par}" << endl;

	widths.clear();
	//        Gene    cDNA    type    F/T     func	   eff
	widths << 1250 << 4000 << 5400 << 6500 << 8950  << max_table_width;

	writeRtfTableSNV(stream,widths,true);

	stream << endl << "\\line" << endl;

	//CNV report
	widths.clear();
	widths<< 5100 << 5600 << 6000 << 9000 << max_table_width;
	//Make rtf report, filter genes for processing system target region
	ReportHelper::writeRtfTableCNV(stream,widths);
	stream << endl << "\\pard\\par" << endl;

	widths.clear();
	widths << 1000 << 1500 << 2700 << 5000 << 6000 << 7000 << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs18\\b Medikamente mit CGI-Annotation\\par}" << endl;
	ReportHelper::writeRtfCGIDrugTable(stream,widths);


	stream << "\\page" << endl;
	widths.clear();
	stream << "{\\pard\\sa45\\sb45\\b Erg\\u228;nzende Informationen zum Befund\\b0\\par}" << endl;
	stream << "{\\pard\\par}" << endl;

	//quality parameters
	QDir directory = QFileInfo(snv_filename_).dir();
	//logfile
	directory.setNameFilters(QStringList()<< "*.log");

	//Sort entries, eldest .log file is last element
	QStringList log_files = directory.entryList(QDir::Files,QDir::Time);
	QString log_file = log_files.last();


	//paths for qcML files of single sample
	directory.cdUp();
	QString qcml_file_tumor_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + tumor_id_ + "/" + tumor_id_ + "_stats_map.qcML";
	QString qcml_file_normal_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + normal_id_ + "/" + normal_id_ + "_stats_map.qcML";

	QCCollection qcml_data_tumor = QCCollection::fromQCML(qcml_file_tumor_stats_map_absolute_path);
	QCCollection qcml_data_normal = QCCollection::fromQCML(qcml_file_normal_stats_map_absolute_path);

	QSharedPointer<QFile> lf = Helper::openFileForReading(QFileInfo(snv_filename_).absolutePath().append("/").append(log_file));
	//extract analysis pipeline version
	QByteArray pipeline_version = lf->readLine().split('\t').at(2).trimmed();
	pipeline_version.remove(0,6);

	widths.clear();
	widths << 3000 << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs18\\b Qualit\\u228;tsparameter:\\par}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Revision der Analysepipeline: " << "\\cell " << pipeline_version << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Coverage Tumor 100x:\\cell"<< begin_table_cell << qcml_data_tumor.value("QC:2000030",true).toString()  << "\%"<< "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Durchschnittliche Tiefe Tumor:\\cell" << begin_table_cell << qcml_data_tumor.value("QC:2000025",true).toString() << "x" << "\\cell" << "\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Coverage Normal 100x:\\cell"<< begin_table_cell << qcml_data_normal.value("QC:2000030",true).toString() << "\%" << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Durchschnittliche Tiefe Normal:\\cell" << begin_table_cell << qcml_data_normal.value("QC:2000025",true).toString() << "x" << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Prozessierungssystem Tumor:\\cell" << begin_table_cell <<  db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true).name << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Prozessierungssystem Normal:\\cell" << begin_table_cell << db_.getProcessingSystemData(db_.processedSampleId(normal_id_), true).name << "\\cell\\row}" << endl;

	stream << "\\pard\\par" << endl;
	//gaps
	stream <<"{\\pard\\b\\sa45\\sb45\\fs18 L\\u252;ckenstatistik:\\par}" << endl;

	widths.clear();
	widths << 2200 << max_table_width;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion:\\cell" << begin_table_cell << QFileInfo(target_region_).fileName()<<"\\cell" << "\\row}" << endl;

	if (!genes_in_target_region_.isEmpty())
	{
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "Zielregion Gene (" << QString::number(genes_in_target_region_.count()) << "): " << "\\cell" << begin_table_cell << genes_in_target_region_.join(", ") << "\\cell" << "\\row}"<< endl;
	}
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion Regionen:\\cell" << begin_table_cell << roi_.count()<<"\\cell" << "\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion Basen:\\cell" << begin_table_cell << roi_.baseCount() << "\\cell" << "\\row}" << endl;

	QString low_cov_file = snv_filename_;
	low_cov_file.replace(".GSvar", "_stat_lowcov.bed");
	BedFile low_cov;
	low_cov.load(low_cov_file);
	low_cov.intersect(roi_);

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell <<"L\\u252;cken Regionen:\\cell"<< begin_table_cell << low_cov.count() << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "L\\u252;cken Basen:\\cell" << begin_table_cell << low_cov.baseCount() << " (" << QString::number(100.0 * low_cov.baseCount()/roi_.baseCount(), 'f', 2) << "%)"<< "\\sa20\\cell" << "\\row}" <<endl;

	QHash<QByteArray, BedFile> grouped = gapStatistics();
	widths.clear();
	widths << 900 << 1600 << 2200 << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\fi20 Gen\\cell" << begin_table_cell_bold << "\\fi20 L\\u252;cken\\cell" << begin_table_cell_bold << "\\fi20 Chr\\cell" << begin_table_cell_bold << "\\fi20 Koordinaten (GRCh37)" << "\\cell" <<"\\row}" << endl;

	for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
	{
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
		stream << begin_table_cell << endl;
		const BedFile& gaps = it.value();
		QString chr = gaps[0].chr().strNormalized(true);
		QStringList coords;
		for (int i=0; i<gaps.count(); ++i)
		{
			coords << QString::number(gaps[i].start()) + "\\_" + QString::number(gaps[i].end());
		}
		stream << "\\fi20 "<< it.key() << "\\cell" << "\\hyphpar "<< begin_table_cell<< "\\fi20 " << gaps.baseCount() << "\\cell"<< begin_table_cell << "\\fi20 "<< chr << "\\cell"<< begin_table_cell << "\\fi20 "<<  coords.join(", ") << "\\hyphparo " <<  endl;
		stream << "\\cell" << "\\row}" << endl;
	}
	widths.clear();
	widths << 1500 << max_table_width;

	stream << "{\\pard\\par}" << endl;
	stream << "{\\pard\\b\\sa45\\sb45\\ Erl\\u228;uterungen zu den Varianten-Tabellen\\par}" << endl;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18 SNV-Tabelle:" << "\\cell";
	stream << "\\fs18\\qj "<<endl;
	stream << "Die Tabelle mit Einzelnukleotid-Varianten (SNVs) zeigt alle in der Tumorprobe gefundenen Splei\\u223;- und proteinkodierenden Varianten. ";
	stream << "Weitere SNVs (wie z.B. Intron-Varianten) sind auf Nachfrage erh\\u228;ltlich. ";
	stream << "Die funktionellen Einsch\\u228;tzungen wurden mithilfe der CancerGenomeInterpreter.org-Datenbank generiert.";
	stream << "\\sa50\\cell\\row}" << endl;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18 CNV-Tabelle:" << "\\cell";
	stream << begin_table_cell << "\\fs18 " << "\\qj ";
	stream << "Die Tabelle mit Kopienzahlvarianten (CNVs) zeigt das Ergebnis der CNV-Analysen, die mit CNVHunter (www.github.com/imgag/ngs-bits) durchgef\\u252;hrt wurden. " << endl;
	stream << "Zur Validierung relevanter Ver\\u228;nderungen empfehlen wir eine zweite, unabh\\u228;ngige Methode. " <<endl;
	stream << "Zu jeder CNV werden die amplifizierten/deletierten Gene aus der Zielregion des Prozessierungssystems angegeben. Wenn ein Gen fett gedruckt ist, wurde dieses von CGI als Tumortreiber beurteilt. " << endl;
	stream << "Die restlichen Gene wurden von CGI als Passenger klassifiziert. " << endl;
	stream << "Die gesch\\u228;tzte Copy Number ist abh\\u228;ngig von Tumorgehalt und Verteilung der CNV-tragenden Zellen im Tumorgewebe." << endl;
	stream << "\\sa50\\cell\\row}" << endl;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18 Medikamente:" << "\\cell" << endl;
	stream << begin_table_cell << "\\fs18\\qj " << "Die Liste zeigt Medikamente, die zu der spezifischen Ver\\u228;nderung in der CGI-Datenbank gelistet sind. ";
	stream << "Es sind auch Medikamente gelistet, die sich auf einen anderen Tumortyp mit derselben Ver\\u228;nderung beziehen. ";
	stream << "Bitte beachten Sie, dass die aufgef\\u252;hrten Medikamente keinesfalls das Urteil eines Arztes ersetzen k\\u246;nnen. ";
	stream << "\\sa50\\cell\\row}" << endl;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18 Mutationslast: " << "\\cell";
	stream << begin_table_cell << "\\fs18\\qj "<<endl;
	stream << "Bei Melanomen gelten Werte unter 3.3 Var/Mbp als niedrig, Werte zwischen 3.3 Var/Mbp und unter 23.1 Var/Mbp als mittel und Werte ab ";
	stream << "23.1 Var/Mbp als hoch. Siehe hierzu die grundlegende Publikation DOI:10.1158/2326-6066.CIR-16-0143. ";
	stream << "Die Mutationslast kann bei der Entscheidung f\\u252;r eine Immuntherapie von Bedeutung sein.";
	stream << "\\cell\\row}" << endl;



	//close stream
	stream << "}";
	outfile->close();
}
