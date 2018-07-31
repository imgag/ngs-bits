#include "ReportHelper.h"
#include "BasicStatistics.h"
#include "OntologyTermCollection.h"
#include "Helper.h"
#include "GenLabDB.h"
#include "TSVFileStream.h"
#include "NGSHelper.h"

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

void RtfTools::writeRtfTableSingleRowSpec(QTextStream& stream, const QList<int>& col_widths, bool border)
{
	int col_numbers = col_widths.size();
	stream << "{\\trowd\\trql";
	if(border == true)
	{
		//Generate cells with widths out of colWidths
		for(int column=0;column<col_numbers;++column)
		{
			stream << "\\clbrdrt\\brdrw15\\brdrs\\clbrdrl\\brdrw15\\brdrs\\clbrdrb\\brdrw15\\brdrs\\clbrdrr\\brdrw15\\brdrs\\cellx" <<col_widths[column];
		}
		stream << endl;
	}
	else
	{
		//Generate cells with widths out of colWidths
		for(int column=0;column<col_numbers;++column)
		{
			stream << "\\cellx" << col_widths[column];
		}
		stream << endl;
	}
}

void RtfTools::writeRtfTableSingleRowSpec(QTextStream &stream, const QList<int> &col_widths, QList<int> borders)
{
	int col_numbers = col_widths.size();

	if(borders.size() != 4) return;
	int b_top = borders[0];
	int b_right = borders[1];
	int b_bottom = borders[2];
	int b_left = borders[3];

	stream << "{\\trowd\\trql";

	for(int column=0;column<col_numbers;++column)
	{
		stream << "\\clbrdrt\\brdrw" << b_top << "\\brdrs\\clbrdrl\\brdrw" << b_left;
		stream << "\\brdrs\\clbrdrb\\brdrw" << b_bottom << "\\brdrs\\clbrdrr\\brdrw" << b_right << "\\brdrs\\cellx" << col_widths[column];
	}
	stream << endl;
}

void RtfTools::writeRtfWholeTable(QTextStream& stream, const QList< QList<QString> >& table, const QList<int>& col_widths, int font_size, bool border, bool bold)
{
	int rows = table.length();
	for(int i=0; i<rows; ++i)
	{
		RtfTools::writeRtfRow(stream, table.at(i), col_widths, font_size, border, bold);
	}
}

void RtfTools::writeRtfRow(QTextStream& stream, const QList<QString>& columns, const QList<int>& col_widths,int font_size, QList<int> borders,bool bold)
{
	QString begin_table_cell = "\\pard\\intbl\\fs" + QString::number(font_size) + " ";
	if(bold)
	{
		begin_table_cell = begin_table_cell + "\\b ";
	}

	RtfTools::writeRtfTableSingleRowSpec(stream,col_widths,borders);
	for(int i=0; i<columns.length(); ++i)
	{
		stream << begin_table_cell << columns.at(i) << "\\cell";
	}
	stream << "\\row}" << endl;
}

void RtfTools::writeRtfRow(QTextStream& stream, const QList<QString>& columns, const QList<int>& col_widths, int font_size, bool border, bool bold)
{
	QString begin_table_cell = "\\pard\\intbl\\fs" + QString::number(font_size) + " ";
	if(bold)
	{
		begin_table_cell = begin_table_cell + "\\b ";
	}

	RtfTools::writeRtfTableSingleRowSpec(stream,col_widths,border);
	for(int i=0; i<columns.length(); ++i)
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

	if(important_snvs.count() == 0) return;

	//set widths
	int max_table_width = colWidths.last();

	QList<int> widths;

	QString begin_table_cell = "\\pard\\intbl\\fs18 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";

	//annotation indices
	int i_tum_dp = important_snvs.annotationIndexByName("tumor_dp",true,true);
	int i_tum_af = important_snvs.annotationIndexByName("tumor_af",true,true);

	widths = colWidths;

	/**********************************
	 * MUTATION BURDEN AND MSI STATUS *
	 *********************************/
	if(!display_germline_hint)
	{
		QList<int> widths = {1250,5000};
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);

		//mutation burden
		stream << begin_table_cell << "Mutationslast:" << "\\cell" << begin_table_cell << mutation_burden_ << " Var/Mbp";
		QString mutation_burden_assesment;
		if(mutation_burden_<3.3) mutation_burden_assesment = "niedrig";
		if(mutation_burden_<=23.1 && mutation_burden_>=3.3) mutation_burden_assesment = "mittel";
		if(mutation_burden_>23.1) mutation_burden_assesment = "hoch";
		stream << " (" << mutation_burden_assesment << ") " << "\\cell" << "\\row}" <<endl;

		//MSI status
		stream << begin_table_cell << "MSI-Status:" << "\\cell" <<begin_table_cell;
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		QByteArray msi_status = "";

		try
		{
			TSVFileStream msi_file(mantis_msi_path_);
			//Use step wise difference (-> stored in the first line of MSI status file) for MSI status
			QByteArrayList data = msi_file.readLine();

			QByteArray step_wise_diff = data[1];

			msi_status += step_wise_diff;

			if(data[3] == "Stable") msi_status += " (stabil)";
			else if(data[3] == "Unstable") msi_status += " (instabil)";

		}
		catch(...)
		{
			 msi_status =  "n/a";
		}
		stream << msi_status;
		stream << "\\cell\\row}" << endl;
	}

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
	for(int i=0; i<important_snvs.count(); ++i)
	{
		QList<QString> columns;
		//gene
		QByteArray gene_name;
		if(!important_snvs[i].annotations().at(snv_index_cgi_gene_).isEmpty())
		{
			gene_name = important_snvs[i].annotations().at(snv_index_cgi_gene_);
		}
		else
		{
			gene_name = important_snvs[i].annotations().at(snv_index_gene_);
		}
		columns.append("\\ql\\li20\\i "+ gene_name + "\\i0");


		/*************************************
		 * Choose coding/splicing transcript *
		 ************************************/
		VariantTranscript use_transcript = selectSomaticTranscript(important_snvs[i]);

		/************************
		 * Parse text for cfDNA *
		 ***********************/
		QByteArray mutation = use_transcript.hgvs_c;

		//for deletions: remove bases according to HGVS nomenclature
		if(mutation.contains("del"))
		{
			int pos = mutation.indexOf("del")+3;
			mutation.truncate(pos);
		}

		QByteArray protein_change = use_transcript.hgvs_p;
		if(protein_change.isEmpty())
		{
			protein_change = "p.?";
		}

		QString cdna = mutation + ":" + protein_change + "\\line " + "(" + use_transcript.id + ")";
		columns.append("\\li20 " + cdna);

		//type: print coding and splicing only
		QByteArray types_all = use_transcript.type;
		types_all = types_all.replace("_variant","");
		types_all = types_all.replace("&", ", ");
		columns.append("\\li20 " + types_all);

		//F/T Tumor
		columns.append("\\qc " + QString::number(important_snvs[i].annotations().at(i_tum_af).toDouble(), 'f', 3) + " / " + important_snvs[i].annotations().at(i_tum_dp));

		/*************************
		* Parse driver statement *
		*************************/
		QByteArray driver_statement = important_snvs[i].annotations().at(snv_index_cgi_driver_statement_);
		QByteArrayList variant_types = use_transcript.type.split('&');
		bool is_cgi_transcript;
		QString cgi_transcript = important_snvs[i].annotations().at(snv_index_cgi_transcript_); //Ensemble-ID of CGI transcript
		cgi_transcript = cgi_transcript.remove(',');
		cgi_transcript = cgi_transcript.trimmed();

		if(cgi_transcript == use_transcript.id)
		{
			is_cgi_transcript = true;
		}
		else
		{
			is_cgi_transcript = false;
		}

		if(is_cgi_transcript && variant_types.count() == 1 )
		{
			if(driver_statement.contains("known"))
			{
				//Make list of cancer acronyms
				QByteArrayList temp_cancer_acronyms = driver_statement.split(':')[1].split(';');
				for(int j=0;j<temp_cancer_acronyms.count();++j)
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

			//remove text "tier" in case of (predicted) passengers
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


		/*******************
		 * Parse gene role *
		 ******************/
		//oncogene: tumor driver and Activating
		if(is_cgi_transcript && variant_types.count() == 1)
		{
			bool is_driver = important_snvs[i].annotations().at(snv_index_cgi_driver_statement_).contains("driver") ||
					important_snvs[i].annotations().at(snv_index_cgi_driver_statement_).contains("known");

			QByteArray gene_role = important_snvs[i].annotations().at(snv_index_cgi_gene_role_);

			if(is_driver && gene_role == "Act")
			{
				columns.append("\\qc Activating");
			}
			else if(is_driver && gene_role == "LoF")
			{
				columns.append("\\qc Inactivating");
			}
			else if(is_driver && gene_role ==("ambiguous"))
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

	/*******************
	 * Order SNV table *
	 ******************/
	//add drivers to final list
	QList< QList<QString> > somatic_snv_table;
	foreach(QList<QString> row, somatic_snv_table_unordered)
	{
		bool is_driver = false;

		for(int i=0; i<row.count(); ++i)
		{
			if(row[i].contains("driver") || row[i].contains("known")) is_driver = true;
		}
		if(is_driver)
		{
			somatic_snv_table.append(row);
		}
	}
	//add passengers to final list
	foreach(QList<QString> row,somatic_snv_table_unordered)
	{
		bool is_driver =false;
		for(int i=0; i<row.count(); ++i)
		{
			if(row[i].contains("driver") || row[i].contains("known")) is_driver = true;
		}

		if(!is_driver && !row[4].contains("NA"))
		{
			somatic_snv_table.append(row);
		}
	}
	//add unclear driver statements
	foreach(QList<QString> row,somatic_snv_table_unordered)
	{
		if(row[4].contains("NA"))
		{
			somatic_snv_table.append(row);
		}
	}

	/*******************
	 * Write RTF table *
	 ******************/
	widths.clear();
	widths = colWidths;
	RtfTools::writeRtfWholeTable(stream,somatic_snv_table,widths,18,true,false);

	if(display_germline_hint)
	{
		widths.clear();
		widths << max_table_width;
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "\\fs18\\sb20 "<<endl;
		stream <<"\\qj\\fs16\\qj\\b Abk\\u252;rzungen:\\b0  " << "{\\i cDNA:} cDNA Position und Auswirkung auf Peptid; {\\i F/T Tumor:} Allelfrequenz/Tiefe der Tumorprobe; ";
		stream << "{\\i Funktion:} Funktionelle Einsch\\u228;tzung der Variante aus CancerGenomeInterpreter.org. \"Known driver\" bedeutet, dass die Ver\\u228;nderung in bestimmten Tumortypen bekannt ist. \"Predicted driver\" bedeutet, dass die Variante mittels probabilistischer Methoden als Tumortreiber vorhergesagt wurde. \"NA\" bedeutet, dass keine Einsch\\u228;tzung vorliegt. ";
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
		for(int i=0; i<cancer_acronyms.count(); ++i)
		{
			stream << cancer_acronyms[i] << " - " << acronyms_to_german.value(cancer_acronyms[i]);
			if(i<cancer_acronyms.count()-1)
			{
				stream << ", ";
			}
		}
		stream << "\\cell" << "\\row}" << endl;
	}

	if(display_germline_hint)
	{
		widths.clear();
		widths << max_table_width;
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "\\sb20\\qj\\fs16\\qj\\b " << "Keimbahnvarianten:\\b0 ";
		stream << "{\\highlight3  Siehe Zusatzbefund / Es wurden {\\b\\ul keine} pathogenen Keimbahnvarianten in den untersuchten Genen ";
		for(int i=0; i<germline_genes_in_acmg_.count(); ++i)
		{
			stream << germline_genes_in_acmg_[i];
			if(i<germline_genes_in_acmg_.count()-1)
			{
				stream << ", ";
			}
		}
		stream << " gefunden.}" << endl;

		stream << "\\cell\\row}" << endl;
	}
}

void ReportHelper::writeRtfTableCNV(QTextStream& stream, const QList<int>& colWidths)
{
	int max_table_width = colWidths.last();

	//beginning of rtf cells
	QString begin_table_cell = "\\pard\\intbl\\fs18 ";

	QList<int> widths;

	//widths if cnv_type is set in input VariantLists
	QList<int> widths_cnv_type;
	widths_cnv_type << 4500 << 5000 << 5400  << 8700 << max_table_width;
	//CNVs
	widths << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs18\\b Kopienzahlvarianten (CNVs)\\b0\\par}" << endl;


	widths.clear();
	widths = colWidths;

	QList< QList<QString> > header_cnvs;
	QList<QString> header_columns;

	header_columns <<"\\trhdr\\qc Gene" << "\\qc CNV" << "\\qc CN" << "\\qc Typ" << "\\qc Gr\\u246;\\u223;e [kb]";
	header_cnvs << header_columns;

	RtfTools::writeRtfWholeTable(stream,header_cnvs,widths,18,true,true);

	//construct 2D QStringList to create RTF table
	QList< QList<QString> > somatic_cnv_table;
	//neccessary for filtering for target region
	//create set with target genes

	QString target_region = processing_system_data.target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region.left(target_region.size()-4) + "_genes.txt");
	target_genes = db_.genesToApproved(target_genes);

	for(int i=0; i<cnvs_filtered_.count(); ++i)
	{
		CopyNumberVariant variant = cnvs_filtered_[i];
		QList<QString> columns;

		//gene names
		//only print genes which which lie in target region
		if(!variant.annotations().at(cnv_index_cgi_genes_).isEmpty())
		{
			//cgi genes
			QByteArrayList cgi_genes = variant.annotations().at(cnv_index_cgi_genes_).split(',');
			//cgi_genes = db_.genesToApproved(cgi_genes);

			QByteArrayList cgi_driver_statements = variant.annotations().at(cnv_index_cgi_driver_statement_).split(',');

			QString genes_included = "";

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
					genes_included.append("\\b\\i " + cgi_genes[j] +"\\b0\\i0 ");
				}
				else
				{
					genes_included.append("\\i " + cgi_genes[j] + "\\i0 ");
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
		else if(cn < 2.)
		{
			columns.append("\\qc DEL");
		}
		else
		{
			columns.append("\\qc{\\highlight3 NA} ");
		}

		//copy numbers
		columns.append("\\qc " + QString::number(cn));

		//coordinates
		columns.append("\\fi20 " +variant.chr().str());
		if(cnv_index_cnv_type_!=-1)
		{
			columns.last().append(" (" + variant.annotations().at(cnv_index_cnv_type_) +")" );
		}

		//size
		columns.append("\\qr " + QString::number(round((cnvs_filtered_[i].end()-cnvs_filtered_[i].start())/1000.)) + "\\ri20");


		somatic_cnv_table.append(columns);
	}

	RtfTools::writeRtfWholeTable(stream,somatic_cnv_table,widths,18,true,false);

	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);

	stream << begin_table_cell << "\\sb20\\fs16\\qj\\b Abk\\u252;rzungen:\\b0  " << "Gene: Gene aus der Zielregion. Tumortreiber sind fett gedruckt und Passenger normal; CNV: Amplifikation (AMP) oder Deletion (DEL); CN: Copy Number; Typ: Chromosomale Position; Gr\\u0246;\\u0223;e: Ausdehnung der CNV in Kilobasen [kb]." ;
	stream << "\\cell\\row}" << endl;

	//Germline
	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\sb20\\qj\\fs16\\qj\\b " << "Keimbahnvarianten:\\b0 ";
	stream << "{\\highlight3  Siehe Zusatzbefund / Es wurden keine pathogenen Keimbahnvarianten gefunden.}" << endl;
	stream << "\\cell\\row}" << endl;

	stream << "{\\pard\\par}" << endl;


	//Summary Amplified/Deleted genes, include passengers and drivers
	GeneSet amplified_cnvs;
	GeneSet deleted_cnvs;

	for(int i=0; i<cnvs_filtered_.count(); ++i)
	{
		CopyNumberVariant variant = cnvs_filtered_[i];

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

	for(int i=0; i<amplified_cnvs.count(); ++i)
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

	for(int i=0; i<deleted_cnvs.count(); ++i)
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

void ReportHelper::writeGapStatistics(QTextStream &stream, const QString& target_file)
{
	QString begin_table_cell = "\\pard\\intbl\\fs18 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";
	int max_table_width = 10000;

	BedFile region_of_interest;
	region_of_interest.load(target_file);

	GeneSet genes_in_target_region = GeneSet::createFromFile(target_file.left(target_file.size()-4)+"_genes.txt");


	stream <<"{\\pard\\b\\sa45\\sb45\\fs18 L\\u252;ckenstatistik:\\par}" << endl;

	QList<int> widths = { 2200 , 10000 };

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion:\\cell" << begin_table_cell << QFileInfo(target_file).fileName()<<"\\cell" << "\\row}" << endl;

	if (!genes_in_target_region.isEmpty())
	{
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "Zielregion Gene (" << QString::number(genes_in_target_region.count()) << "): " << "\\cell" << begin_table_cell << genes_in_target_region.join(", ") << "\\cell" << "\\row}"<< endl;
	}
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion Regionen:\\cell" << begin_table_cell << region_of_interest.count()<<"\\cell" << "\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion Basen:\\cell" << begin_table_cell << region_of_interest.baseCount() << "\\cell" << "\\row}" << endl;

	QString low_cov_file = snv_filename_;
	low_cov_file.replace(".GSvar", "_stat_lowcov.bed");
	BedFile low_cov;
	low_cov.load(low_cov_file);
	low_cov.intersect(region_of_interest);

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell <<"L\\u252;cken Regionen:\\cell"<< begin_table_cell << low_cov.count() << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "L\\u252;cken Basen:\\cell" << begin_table_cell << low_cov.baseCount() << " (" << QString::number(100.0 * low_cov.baseCount()/roi_.baseCount(), 'f', 2) << "%)"<< "\\sa20\\cell" << "\\row}" <<endl;

	QHash<QByteArray, BedFile> grouped = gapStatistics(region_of_interest);

	//write gap statistics for each gene only if there are few genes
	if(grouped.keys().count() < 25)
	{
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
	}
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
		row.drug_ = line.at(i_drug);
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


	//fill in information from erased drugs
	foreach(CGIDrugReportLine erased_drug,erased_drugs)
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
		for(int i=0; i<result_line.count(); ++i)
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

const QList<QByteArray> CGIDrugTable::getAcronyms(int evid_level) const
{
	QList<CGIDrugReportLine> drugs = drugsByEvidenceLevel(evid_level);

	QList<QByteArray> results;

	foreach(CGIDrugReportLine drug,drugs)
	{
		QByteArray temp_tumor_entities = drug.entity().toLatin1();
		temp_tumor_entities.replace(" ","");
		QList<QByteArray> tumor_entities = temp_tumor_entities.split(',');

		for(int i=0; i<tumor_entities.count(); ++i)
		{
			results.append(tumor_entities[i]);
		}
	}

	return results;
}

ReportHelper::ReportHelper()
{
}

ReportHelper::ReportHelper(QString snv_filename, const CnvList& filtered_cnvs, QString target_region, const FilterCascade& filters)
	: snv_filename_(snv_filename)
	, snv_germline_()
	, cnvs_filtered_(filtered_cnvs)
	, db_()
	, target_region_(target_region)
	, filters_(filters)
{

	snv_variants_.load(snv_filename_);

	QString base_name = QFileInfo(snv_filename_).baseName();

	tumor_id_ = base_name.split('-')[0];
	normal_id_ = base_name.split('-')[1].split('_')[0]+'_'+base_name.split('-')[1].split('_')[1];

	cgi_drugs_path_ = QFileInfo(snv_filename_).absolutePath() + "/" + base_name + "_cgi_drug_prescription.tsv";

	//filename MANTIS output
	mantis_msi_path_ = QFileInfo(snv_filename_).absolutePath() + "/" + base_name + "_msi.tsv";

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

	//load genes that lie in target region
	genes_in_target_region_ = GeneSet::createFromFile(target_region_.left(target_region_.size()-4)+"_genes.txt");

	//get cancer type which was used for CGI analysis and text for ICD10-diagnosis
	for(int i=0; i<snv_variants_.comments().count(); ++i)
	{
		if(snv_variants_.comments().at(i).contains("CGI_CANCER_TYPE"))
		{
			cgi_cancer_type_ = snv_variants_.comments().at(i).split('=')[1];
		}
	}

	if(cgi_cancer_type_.isEmpty())
	{
		cgi_cancer_type_ = "empty";
	}

	//assign columns indices for SNV file
	snv_index_filter_ = snv_variants_.annotationIndexByName("filter",true,true);
	snv_index_coding_splicing_ = snv_variants_.annotationIndexByName("coding_and_splicing",true,true);

	snv_index_cgi_driver_statement_ = snv_variants_.annotationIndexByName("CGI_driver_statement",true,true);
	snv_index_cgi_gene_role_ = snv_variants_.annotationIndexByName("CGI_gene_role",true,true);
	snv_index_cgi_transcript_ = snv_variants_.annotationIndexByName("CGI_transcript",true,true);
	snv_index_cgi_gene_ = snv_variants_.annotationIndexByName("CGI_gene",true,true);
	snv_index_gene_ = snv_variants_.annotationIndexByName("gene",true,true);


	cnv_index_cgi_gene_role_ = -1;
	cnv_index_cnv_type_ = -1;
	cnv_index_cgi_genes_ = -1;
	cnv_index_cgi_driver_statement_ = -1;

	for(int i=0; i<cnvs_filtered_.annotationHeaders().count(); ++i)
	{
		if(cnvs_filtered_.annotationHeaders()[i] == "CGI_gene_role")
		{
			cnv_index_cgi_gene_role_ = i;
		}

		if(cnvs_filtered_.annotationHeaders()[i] == "cnv_type")
		{
			cnv_index_cnv_type_ = i;
		}

		if(cnvs_filtered_.annotationHeaders()[i] == "CGI_genes")
		{
			cnv_index_cgi_genes_ = i;
		}

		if(cnvs_filtered_.annotationHeaders()[i] == "CGI_driver_statement")
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

	//load qcml data
	QString qcml_file = base_name;
	qcml_file.append("_stats_som.qcML");
	QString qcml_file_absolute_path = QFileInfo(snv_filename).absolutePath().append("/").append(qcml_file);
	qcml_data_ = QCCollection::fromQCML(qcml_file_absolute_path);
	roi_.load(target_region);

	processing_system_data = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true);

	//load metadata from GenLab
	//get histological tumor proportion and diagnosis from GenLab
	GenLabDB db_genlab;
	icd10_diagnosis_code_ = db_genlab.diagnosis(tumor_id_.split('_')[0]);
	if(icd10_diagnosis_code_.isEmpty() || icd10_diagnosis_code_ == "n/a") //try to resolve diagnosis with full tumor ID
	{
		icd10_diagnosis_code_ = db_genlab.diagnosis(tumor_id_);
	}
	histol_tumor_fraction_ = db_genlab.tumorFraction(tumor_id_.split('_')[0]);
	if(histol_tumor_fraction_.isEmpty() || histol_tumor_fraction_ == "n/a") //try to resolve tumor fraction with full tumor ID
	{
		histol_tumor_fraction_ = db_genlab.tumorFraction(tumor_id_);
	}

	try
	{
		mutation_burden_ = qcml_data_.value("QC:2000053",true).asString().split(' ')[1].remove('(').toDouble();
	}
	catch(...)
	{
		mutation_burden_ = std::numeric_limits<double>::quiet_NaN();
	}
	germline_genes_in_acmg_ = GeneSet::createFromFile(db_.getTargetFilePath(true,true) + "ACMG_25Onkogene_20_genes.txt");
}

VariantList ReportHelper::filterSnvForCGIAnnotation(bool filter_for_target_region)
{
	//save variants which are included in SNV report in this VariantList
	VariantList important_snvs;
	important_snvs.copyMetaData(snv_variants_);

	//Skip unimportant SNVs: CGI, filtered, OBO type
	for(int i=0; i<snv_variants_.count(); ++i)
	{
		Variant variant = snv_variants_[i];
		QByteArray cgi_gene = variant.annotations().at(snv_index_cgi_gene_);

		//skip variants which do not lie in target region
		if(filter_for_target_region && !genes_in_target_region_.contains(cgi_gene)) continue;

		//apply filters: keep variants
		bool keep = false;

		QList<QString> filter_entries = QString::fromUtf8(variant.annotations().at(snv_index_filter_)).split(';');

		foreach(QString filter,filter_keep_)
		{
			if(filter_entries.contains(filter))
			{
				keep = true;
				break;
			}
		}
		bool remove = false;
		foreach(QString filter,filter_remove_)
		{
			if(filter_entries.contains(filter))
			{
				remove = true;
				break;
			}
		}

		if(!keep)
		{
			if(remove) continue;
		}


		bool filtering = false;
		foreach(QString filter,filter_filter_)
		{
			if(!filter_entries.contains(filter))
			{
				filtering = true;
			}
		}
		if(filtering) continue;
		important_snvs.append(variant);
	}

	return important_snvs;
}

VariantList ReportHelper::filterSnVForGermline()
{
	int i_1000g = snv_germline_.annotationIndexByName("1000g",true,true);
	int i_exac = snv_germline_.annotationIndexByName("ExAC",true,true);
	int i_gnomAD = snv_germline_.annotationIndexByName("gnomAD",true,true);
	int i_cgi_driver_statement = snv_germline_.annotationIndexByName("CGI_driver_statement",true,true);
	int i_gene = snv_germline_.annotationIndexByName("gene",true,true);

	VariantList result_list;
	result_list.copyMetaData(snv_germline_);

	for(int i=0;i<snv_germline_.count();++i)
	{
		Variant snv = snv_germline_[i];

		bool is_polymorphism = snv.annotations().at(i_1000g).toDouble() > 0.01 || snv.annotations().at(i_exac).toDouble() > 0.01 || snv.annotations().at(i_gnomAD).toDouble() > 0.01;
		bool is_driver = snv.annotations().at(i_cgi_driver_statement).contains("known") || snv.annotations().at(i_cgi_driver_statement).contains("driver");

		if(is_polymorphism && !is_driver) continue;

		QByteArrayList genes = snv.annotations().at(i_gene).split(',');
		bool in_acmg = false; //gene is in acmg list with important germline variants
		foreach(QByteArray gene_of_interest,germline_genes_in_acmg_)
		{
			if(genes.contains(gene_of_interest))
			{
				in_acmg = true;
				break;
			}
		}
		if(!in_acmg && !is_driver) continue;

		result_list.append(snv);
	}
	return result_list;
}

QHash<QByteArray, BedFile> ReportHelper::gapStatistics(const BedFile region_of_interest)
{
	BedFile roi_inter;
	QString processed_sample_id = db_.processedSampleId(tumor_id_);
	ProcessingSystemData system_data = db_.getProcessingSystemData(processed_sample_id, true);
	roi_inter.load(system_data.target_file);
	roi_inter.intersect(region_of_interest);
	roi_inter.merge();

	//Import BedFile with low coverage statistics
	QString low_cov_file = snv_filename_;
	low_cov_file.replace(".GSvar", "_stat_lowcov.bed");
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
	if(cgi_cancer_type_ == "CANCER" || cgi_cancer_type_ == "")
	{
		stream << "{\\fs18 Cannot create drug table because CGI tumor type is set to " << cgi_cancer_type_ << "." << endl;
		stream << "Please restart CGI analysis with a properly selected cancer type.}" << endl;
		return;
	}
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

	drugs.mergeDuplicates(1);
	drugs.mergeDuplicates(2);
	drugs.mergeDuplicates(3);
	drugs.mergeDuplicates(4);
	drugs.mergeDuplicates(5);
	drugs.mergeDuplicates(6);

	//Make list of copy number altered genes which shall be kept in drug list
	CnvList cnvs = cnvs_filtered_;
	GeneSet keep_cnv_genes;
	for(int i=0;i<cnvs.count();++i)
	{
		keep_cnv_genes << cnvs[i].genes();
	}


	//Make list of SNPs genes which shall be kept. A complete match control is not possible because CGI does not deliver base change or transcript
	//IDs in the drug annotation file
	VariantList snvs = filterSnvForCGIAnnotation(false);
	GeneSet keep_snv_genes;
	for(int i=0;i<snvs.count();++i)
	{
		keep_snv_genes << snvs[i].annotations().at(snv_index_cgi_gene_);
	}

	QList< QList<QString > > drugs_as_string;
	QList<CGIDrugReportLine> drugs_sorted = drugs.drugsSortedPerGeneName();

	QStringList acronyms;

	foreach(CGIDrugReportLine drug,drugs_sorted)
	{
		//filter genes which do not appear in SNV and CNV mutation list
		QStringList genes = drug.gene().split(',');
		bool gene_is_in_cnv_list = true;
		bool gene_is_in_snv_list = true;
		foreach(QString gene,genes)
		{
			if((drug.alterationType().contains("AMP") || drug.alterationType().contains("DEL"))
				&& !keep_cnv_genes.contains(gene.toLatin1()))
			{
				gene_is_in_cnv_list = false;
			}
			if(drug.alterationType().contains("MUT") && !keep_snv_genes.contains(gene.toLatin1()))
			{
				gene_is_in_snv_list = false;
			}
		}
		if(!gene_is_in_cnv_list) continue;
		if(!gene_is_in_snv_list) continue;

		//extend list of cancer acronyms that shall appear in explanation
		acronyms.append(drug.entity().split(','));

		//Alteration type / tumor type / drug / effect / evidence /source
		QList<QString> result_line = drug.asStringList();

		//remove first column because gene name is included in alteration type already
		result_line.removeAt(0);

		//Parse for RTF table
		bool aa_change_in_variant_list = false;

		//check whether there are variants which correspond to the same aa change in SNV list
		for(int j=0;j<snvs.count();++j)
		{
			Variant snv = snvs[j];

			if(result_line[0].split(':')[0] != snv.annotations().at(snv_index_cgi_gene_)) continue;

			foreach(VariantTranscript trans, snv.transcriptAnnotations(snv_index_coding_splicing_))
			{
				if(trans.hgvs_p.contains(result_line[0].split(':')[1].toLatin1())
				   || trans.type.contains(result_line[0].split(':')[1].toLatin1()))
				{
					aa_change_in_variant_list = true;
					break;
				}
			}
			if(aa_change_in_variant_list) break;
		}

		//highlight AA changes which could not be identified in SNV list
		if(!aa_change_in_variant_list && !drug.alterationType().contains("DEL") && !drug.alterationType().contains("AMP"))
		{
			result_line[0].prepend("\\highlight3 ");
		}

		for(int i=0; i<result_line.count(); ++i)
		{
			result_line[i].prepend("\\li20 ");
		}

		//gene names should be printed italic
		result_line[0].prepend("\\i ");
		result_line[0].replace(":","\\i0 :");
		result_line[0].replace(",",",\\i ");
		result_line[0].append("\\i0 ");

		drugs_as_string.append(result_line);
	}

	RtfTools::writeRtfWholeTable(stream,drugs_as_string,widths,18,true,false);

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

	acronyms.removeDuplicates();
	std::sort(acronyms.begin(),acronyms.end());
	for(int i=0; i<acronyms.count(); ++i)
	{
		stream << acronyms[i] << " - " << acronyms_to_german.value(acronyms[i].toLatin1());
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

	for(int i=0; i<variants.count(); ++i)
	{
		Variant variant = variants[i];

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

		bool is_coding_splicing = transcript.isPartOntologyTerms(obo_terms_coding_splicing_);
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

	CnvList variants = cnvs_filtered_;

	QString target_region_processing_system = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_), true).target_file;
	GeneSet target_genes = GeneSet::createFromFile(target_region_processing_system.left(target_region_processing_system.size()-4) + "_genes.txt");
	NGSD db;
	target_genes = db.genesToApproved(target_genes);

	for(int i=0; i<variants.count(); ++i)
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
		for(int i=0; i<variant.copyNumbers().count(); ++i)
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

	for(int i=0; i<snv_variants_.count(); ++i)
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

VariantTranscript ReportHelper::selectSomaticTranscript(const Variant& variant)
{
	QString cgi_transcript = variant.annotations().at(snv_index_cgi_transcript_); //Ensemble-ID of CGI transcript
	cgi_transcript = cgi_transcript.remove(',');
	cgi_transcript = cgi_transcript.trimmed();
	VariantTranscript use_transcript; //transcript to be used

	//choose transcript which has the same Ensemble ID as CGI and is coding splicing
	QList<VariantTranscript> transcripts = variant.transcriptAnnotations(snv_index_coding_splicing_);

	foreach(const VariantTranscript& trans, transcripts)
	{
		if(trans.id != cgi_transcript) continue;

		if(trans.isPartOntologyTerms(obo_terms_coding_splicing_))
		{
			use_transcript = trans;
			break;
		}
	}

	//take first co/sp transcript if CGI transcript was not found or not coding/splicing
	if(use_transcript.id.isEmpty())
	{
		foreach(const VariantTranscript& trans,transcripts)
		{
			if(trans.isPartOntologyTerms(obo_terms_coding_splicing_))
			{
				use_transcript = trans;
				break;
			}
		}
	}

	//take first transcript if no coding/splicing transcript was found
	if(use_transcript.id.isEmpty()) use_transcript = transcripts[0];

	return use_transcript;
}

void ReportHelper::writeRtfTableGermlineSNV(QTextStream &stream, const QList<int> &colWidths)
{

	//Column indices in germline SNV file
	int i_ExAC = snv_germline_.annotationIndexByName("ExAC",true,true);
	int i_1000g = snv_germline_.annotationIndexByName("1000g",true,true);
	int i_gnomAD = snv_germline_.annotationIndexByName("gnomAD",true,true);
	int i_cgi_driver_statement;
	int i_cgi_gene;
	int i_genotype;

	VariantList filtered_germline_snv;
	try
	{
		filtered_germline_snv = filterSnVForGermline();
		i_cgi_driver_statement = snv_germline_.annotationIndexByName("CGI_driver_statement",true,true);
		i_cgi_gene = snv_germline_.annotationIndexByName("CGI_gene",true,true);
		i_genotype = snv_germline_.annotationIndexByName("genotype",true,true);
	}
	catch(ArgumentException e)
	{
		QMessageBox::warning(NULL,"Germline variant file","Could not parse control tissue variants for somatic report: " + e.message());
		return;
	}

	QList< QList<QString> > snv_table;

	for(int i=0;i<filtered_germline_snv.count();++i)
	{
		Variant variant = filtered_germline_snv[i];

		QList<QString> row;

		row.append(variant.chr().str());
		row.append(QString::number(variant.start()));
		row.append(variant.ref());
		row.append(variant.obs());

		row.append(variant.annotations().at(i_genotype));
		row.append(variant.annotations().at(i_ExAC));
		row.append(variant.annotations().at(i_1000g));
		row.append(variant.annotations().at(i_gnomAD));

		row.append(variant.annotations().at(i_cgi_gene));
		row.append(variant.annotations().at(i_cgi_driver_statement));

		snv_table.append(row);
	}

	RtfTools::writeRtfWholeTable(stream,snv_table,colWidths,18,true,false);
}

void ReportHelper::writeRtf(const QString& out_file)
{
	QString begin_table_cell = "\\pard\\intbl\\fs18 ";

	//max table widths in rtf file
	int max_table_width = 9921;
	QList<int> widths;
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(out_file);
	QTextStream stream(outfile.data());
	RtfTools::writeRtfHeader(stream);

	//create SNV table with selected SNVs only
	//        Gene    cDNA    type    F/T     func	   eff
	widths << 1250 << 4000 << 5400 << 6500 << 8950  << max_table_width;
	writeRtfTableSNV(stream,widths,false);

	stream << endl <<  "\\line" << endl;
	stream << "\\page" << endl;

	/********************
	 * TECHNICAL REPORT *
	 *******************/
	stream << "{\\pard\\sa45\\sb45\\b Technischer Report\\b0\\par}" << endl;
	stream << "{\\pard\\par}" << endl;

	//quality parameters
	QDir directory = QFileInfo(snv_filename_).dir();
	directory.cdUp();
	QString qcml_file_tumor_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + tumor_id_ + "/" + tumor_id_ + "_stats_map.qcML";
	QString qcml_file_normal_stats_map_absolute_path = directory.absolutePath() + "/" + "Sample_" + normal_id_ + "/" + normal_id_ + "_stats_map.qcML";

	QCCollection qcml_data_tumor = QCCollection::fromQCML(qcml_file_tumor_stats_map_absolute_path);
	QCCollection qcml_data_normal = QCCollection::fromQCML(qcml_file_normal_stats_map_absolute_path);

	/**********************
	 * QUALITY PARAMETERS *
	 *********************/
	widths.clear();
	widths << 3000 << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs18\\b Qualit\\u228;tsparameter:\\par}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Analysepipeline: " << "\\cell " << snv_variants_.getPipeline() << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Auswertungssoftware: " << "\\cell " << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << "\\cell" << "\\row}" <<endl;
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
	writeGapStatistics(stream,target_region_);

	stream <<"\\page" << endl;

	widths.clear();
	widths << 3000 << max_table_width;

	stream << "{\\pard\\b Weiterf\\u252;hrender Bericht\\par}" << endl;

	/***********************
	 * GENERAL INFORMATION *
	 **********************/
	stream << "{\\pard\\par}" << endl;
	stream << "{\\pard\\sa45\\fs18\\b Allgemeine Informationen:\\par}" << endl;

	double tumor_molecular_proportion;
	try
	{
		 tumor_molecular_proportion = qcml_data_.value("QC:2000054",true).toString().toDouble();
	}
	catch(...)
	{
		tumor_molecular_proportion = std::numeric_limits<double>::quiet_NaN();
	}

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

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "CGI-Tumortyp:\\cell" << begin_table_cell << cgi_cancer_type_ << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);

	stream << begin_table_cell << "Prozessierungssystem:\\cell" << begin_table_cell << processing_system_data.name;
	GeneSet gene_set = GeneSet::createFromFile(processing_system_data.target_file.left(processing_system_data.target_file.size()-4) + "_genes.txt");
	stream << " (" << gene_set.count() << " Gene" << ")";
	stream << "\\cell\\row}" << endl;

	stream << "\\line" << endl;

	/*************
	 * SNV TABLE *
	 ************/
	stream << "{\\pard\\sb45\\sa45\\fs18\\b Weitere somatische Ver\\u228;nderungen (SNVs und kleine INDELs)\\par}" << endl;
	widths.clear();
	//        Gene    cDNA    type    F/T     func	   eff
	widths << 1250 << 4000 << 5400 << 6500 << 8950  << max_table_width;
	writeRtfTableSNV(stream,widths,true);
	stream << endl << "\\line" << endl;

	/*************
	 * CNV TABLE *
	 ************/
	widths.clear();
	widths<< 6300 << 6800 << 7200 << 8900 << max_table_width;
	//Make rtf report, filter genes for processing system target region
	ReportHelper::writeRtfTableCNV(stream,widths);
	stream << endl << "\\pard\\par" << endl;
	stream << "\\page" << endl;


	widths.clear();
	widths << 1000 << 1500 << 2700 << 5000 << 6000 << 7000 << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs18\\b Medikamente mit CGI-Annotation\\par}" << endl;

	ReportHelper::writeRtfCGIDrugTable(stream,widths);

	stream << "\\page" << endl;

	/****************
	 * EXPLANATIONS *
	 ***************/
	widths.clear();
	widths << 1500 << max_table_width;

	stream << "{\\pard\\par}" << endl;
	stream << "{\\pard\\b\\sa45\\sb45\\ Erl\\u228;uterungen zu den Tabellen\\par}" << endl;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18 SNV-Tabelle:" << "\\cell";
	stream << "\\fs18\\qj "<<endl;
	stream << "Die Tabelle mit Einzelnukleotid-Varianten (SNVs) zeigt alle in der Tumorprobe gefundenen Splei\\u223;- und proteinkodierenden Varianten, ";
	stream << "deren Allelfrequenz in der Tumorprobe mindestens 5\% betr\\u228;gt. " << endl;
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
	stream << "\\sa50\\cell\\row}" << endl;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18 MSI-Status: " << "\\cell";
	stream << begin_table_cell << "\\fs18\\qj" << endl;
	stream << "Die Abk\\u252;rzung MSI steht f\\u252;r Mikrosatelliteninstabilit\\u228;t. Der MSI-Status wurde mit dem NGS-Tool ";
	stream << "MANTIS (DOI:10.18632/oncotarget.13918) ermittelt. Der Zahlenwert bezieht sich auf die gemittelte schrittweise Differenz der L\\u228;ngen der Tumor-Reads und Normal-Reads aller im Panel enthaltenen MS-Loci. ";
	stream << "Dabei verwenden wir einen Wert von 0.4 als Schwelle, um zwischen MS stabilen- und instabilen Tumoren zu unterscheiden. ";
	stream << "Zur Validierung des MSI-Status empfehlen wir eine weitere, pathologische Untersuchung des Tumorgewebes." << endl;
	stream << "\\cell\\row}" << endl;

	stream << "\\page" << endl;

	stream << "{\\pard\\b\\sa45\\sb45\\ Liste der im Panel enthaltenen Gene\\par}" << endl;

	QString target_bed_file = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_),true).target_file;
	QString target_genes_file = target_bed_file.left(target_bed_file.size()-4) + "_genes.txt";
	QString target_panel_name = db_.getProcessingSystemData(db_.processedSampleId(tumor_id_),true).name;
	GeneSet target_genes = GeneSet::createFromFile(target_genes_file);

	/***********************
	 * LIST OF PANEL GENES *
	 **********************/
	widths.clear();
	widths << max_table_width;
	stream << "\\pard\\fs18 " << "\\b Panel:\\b0  " << target_panel_name <<"\\sa60\\par" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs18\\qj " << endl;
	for(int j=0;j<target_genes.count();++j)
	{
		stream << target_genes[j];
		if(j<target_genes.count()-1) stream << ", ";
	}

	stream << "\\cell\\row}" << endl;

	//close stream
	stream << "}";
	outfile->close();
}
