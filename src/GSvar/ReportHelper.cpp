#include "ReportHelper.h"
#include "BasicStatistics.h"
#include "OntologyTermCollection.h"
#include "VariantFilter.h"
#include "Helper.h"
#include "GenLabDB.h"
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QMainWindow>
#include <QSqlError>

void RtfTools::writeRtfHeader(QTextStream& stream)
{
	//stream.setCodec("ANSI");
	stream << "{\\rtf1\\ansi\\ansicpg1252\\deff0{\\fonttbl{\\f0 Times New Roman;}}{\\colortbl;\\red188\\green230\\blue138;\\red255\\green0\\blue0;}" << "\n";
	stream << "\\paperw12240\\paperh15840\\margl1417\\margr1417\\margt1417\\margb1134" << "\n";
}

void RtfTools::writeRtfTableSingleRowSpec(QTextStream& stream,const QList<int>& col_widths, bool border)
{
	int col_numbers = col_widths.size();
	stream << "{\\trowd\\trgraph70";
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

void RtfTools::writeRtfTableSNV(QTextStream& stream, const QList<int>& colWidths, const VariantList& important_snvs, const VariantList& further_genes, double mutation_burden)
{
	//set widths
	int max_table_width = colWidths.last();

	QList<int> widths;
	widths << max_table_width;

	QString begin_table_cell = "\\pard\\intbl\\fs20 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";

	//annotation indices
	int i_gene = important_snvs.annotationIndexByName("gene",true,true);
	int i_tum_dp = important_snvs.annotationIndexByName("tumor_dp",true,true);
	int i_co_sp = important_snvs.annotationIndexByName("coding_and_splicing",true,true);
	int i_tum_af = important_snvs.annotationIndexByName("tumor_af",true,true);
	int i_cgi_transcript = important_snvs.annotationIndexByName("CGI_transcript",true,true);
	int i_cgi_gene_role = important_snvs.annotationIndexByName("CGI_gene_role",true,true);
	int i_cgi_driver_statement = important_snvs.annotationIndexByName("CGI_driver_statement",true,true);
	//rtf somatic mutation rate
	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\qj{\\b\\fs18 Mutationslast:\\b0  " << mutation_burden << " Varianten/Mbp";
	QString mutation_burden_assesment;
	if(mutation_burden<3.3) mutation_burden_assesment = "niedrig";
	if(mutation_burden<=23.1 && mutation_burden>=3.3) mutation_burden_assesment = "mittel";
	if(mutation_burden>23.1) mutation_burden_assesment = "hoch";
	stream << " (" << mutation_burden_assesment << ")*" << "}" << endl;
	stream << "\\sa20\\cell\\row}" << "\\sa200 "<< endl;
	widths.clear();
	widths = colWidths;

	//rtf somatic SNV table
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\qc " << "Position\\cell" << begin_table_cell_bold << "\\qc " << "R\\cell" << begin_table_cell_bold << "\\qc " << "V\\cell" << begin_table_cell_bold;
	stream << "\\qc " << "F/T Tumor\\cell"  <<begin_table_cell_bold << "\\qc " << "Gen"  << "\\cell "<< begin_table_cell_bold << "\\qc " << "cDNA\\cell" << begin_table_cell_bold << "\\qc " << "Eff."<< "\\cell "<< begin_table_cell_bold <<"\\qc "<< "Evidenz"<<"\\cell" << "\\row}" << endl;
	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\qc " << "Somatische Varianten, die von CGI als Tumortreiber eingestuft wurden:" << "\\cell" << "\\row}" <<endl;

	//construct 2D QStringList to create RTF table
	QList< QList<QString> > somatic_snv_table;
	for(int i=0;i<important_snvs.count();i++)
	{
		QList<QString> columns;
		//position
		columns.append("\\fi20 " + important_snvs[i].chr().str() + ":" + QString::number(important_snvs[i].start()) + "-" + QString::number(important_snvs[i].end()));
		//reference
		columns.append("\\qc " + important_snvs[i].ref());
		//observed
		columns.append("\\qc " + important_snvs[i].obs());
		//F/T Tumor
		columns.append("\\qc " + QString::number(important_snvs[i].annotations().at(i_tum_af).toDouble(), 'f', 3) + "/" + important_snvs[i].annotations().at(i_tum_dp));
		//gene
		columns.append("\\qc "+ important_snvs[i].annotations().at(i_gene));

		//cDNA
		//Only print one transcript: that one from CGI, otherwise the first transcript in the columns
		QByteArrayList hgvs_terms = important_snvs[i].annotations().at(i_co_sp).split(',');
		QStringList tmp_ensemble_names;
		QString cgi_transcript = important_snvs[i].annotations().at(i_cgi_transcript);
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

		QString mutation = important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[5];
		QString protein_change = important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[6];

		if(protein_change.isEmpty())
		{
			protein_change = "p.?";
		}
		QString cdna = mutation + ":" + protein_change;
		columns.append("\\fi20 " + cdna);

		//gene role
		if(important_snvs[i].annotations().at(i_cgi_gene_role) != "ambiguous")
		{
			columns.append("\\qc " + important_snvs[i].annotations().at(i_cgi_gene_role));
		}
		else
		{
			columns.append("\\qc Amb");
		}

		//Parse driver statement
		QByteArray driver_statement;
		if(important_snvs[i].annotations().at(i_cgi_driver_statement).contains("predicted"))
		{
			driver_statement = "vorhergesagt";
		}
		else
		{
			//contains now a list with cancer acronyms in which SNV is known driver
			driver_statement = "bekannt";
		}
		columns.append("\\fi20 " + driver_statement);


		somatic_snv_table.append(columns);
	}
	widths.clear();
	widths = colWidths;
	RtfTools::writeRtfWholeTable(stream,somatic_snv_table,widths,20,true,false);

	widths.clear();
	widths << max_table_width;


	//SNVs classified as passengers: only print gene and protein change
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell << "\\qc\\b " << "Somatische Varianten, die von CGI als Passenger klassifiziert wurden:" << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell << "\\fs18\\qj ";

	//List with SNVs in desired output format
	QStringList results;

	for(int i=0;i<further_genes.count();i++)
	{
		QString result;
		//take CGI transcript if available, otherwise take first transcript
		QByteArray cgi_transcript = further_genes[i].annotations().at(i_cgi_transcript);
		QByteArrayList all_transcripts = further_genes[i].annotations().at(i_co_sp).split(',');
		int keep_number = 0;
		for(int j=0;j<all_transcripts.count();j++)
		{
			if(cgi_transcript == all_transcripts[j])
			{
				keep_number = j;
				break;
			}
		}

		QString gene_name = further_genes[i].annotations().at(i_co_sp).split(',')[keep_number].split(':')[0];

		QString code_change = further_genes[i].annotations().at(i_co_sp).split(',')[keep_number].split(':')[5];
		QString protein_change = further_genes[i].annotations().at(i_co_sp).split(',')[keep_number].split(':')[6];

		result = gene_name + " (" +protein_change;
		if(protein_change.isEmpty())
		{
			result = result + code_change + ":p.?";
		}
		result = result + ")";

		results.append(result);
	}

	if(results.count() > 0)
	{
		std::sort(results.begin(),results.end());
		for(int i=0;i<results.count();i++)
		{
			stream << results[i];

			if(i < results.count()-1)
			{
				stream << ", " << endl;
			}
		}
	}
	else
	{
		stream << "\\qc\\fs20 Keine" << endl;
	}

	stream << "\\cell\\row}" <<endl;


	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\qc " << "Keimbahnvarianten:" << "\\cell" << "\\row}" <<endl;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell << "\\qc ";
	stream << "{\\cf2 Siehe Zusatzbefund / Keine}" << endl;

	stream << "\\cell\\row}" << endl;

	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);

	stream << begin_table_cell << "\\fs16\\qj\\b Abk\\u252;rzungen:\\b0  " << "Position: Chromosomale Position (GRCh37); R: Allel Referenz; V: Allel Variante; F/T Tumor: Frequenz/Tiefe Tumor; cDNA: cDNA Position und Auswirkung auf Peptid; Eff.: Auswirkung der Mutation, LoF (Loss of Function), Act (Activating) oder Amb (Ambiguous); Evidenz: Gibt an, ob die Variante als Treiber bekannt ist oder vorhergesagt wurde."  << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell <<  "\\fs16\\b *Mutationslast:\\b0  Werte von unter 3.3 Var/Mbp gelten als niedrig, Werte zwischen 3.3 Var/Mbp und unter 23.1 Var/Mbp als mittel und Werte ab ";
	stream << "\\fs16 23.1 Var/Mbp gelten als hoch. Die Mutationslast kann bei der Entscheidung ob eine Immuntherapie sinnvoll ist von Bedeutung sein." << "\\cell\\row}" << endl;
	stream << "{\\pard\\par}";

	stream << "\\line" <<endl;
}

void RtfTools::writeRtfTableCNV(QTextStream& stream, const QList<int>& colWidths, const CnvList& important_cnvs, QString target_region)
{
	int max_table_width = colWidths.last();

	//beginning of rtf cells
	QString begin_table_cell = "\\pard\\intbl\\fs20 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";

	QList<int> widths;

	int i_cnv_type = -1;
	for(int i=0;i<important_cnvs.annotationHeaders().count();i++)
	{
		if(important_cnvs.annotationHeaders()[i] == "cnv_type")
		{
			i_cnv_type = i;
		}
	}

	//get indices of CGI_genes and CGI_driver_statement
	// !! column CGI_genes is already filtered for target region !!
	int i_cgi_genes = -1;
	int i_cgi_driver_statement = -1;

	for(int i=0;i<important_cnvs.annotationHeaders().count();i++)
	{

		if(important_cnvs.annotationHeaders()[i] == "CGI_genes")
		{
			i_cgi_genes = i;
		}
		if(important_cnvs.annotationHeaders()[i] == "CGI_driver_statement")
		{
			i_cgi_driver_statement = i;
		}
	}

	if(i_cgi_genes == -1)
	{
		return;
		THROW(FileParseException,"CNV file does not contain a CGI_genes column");
	}
	if(i_cgi_driver_statement == -1)
	{
		return;
		THROW(FileParseException,"CNV file does not contain a CGI_driver_statement column");
	}



	//widths if cnv_type is set in input VariantLists
	QList<int> widths_cnv_type;
	widths_cnv_type << 3000 << 4100 << 5000 << 5600 << 6000 <<max_table_width;

	//CNVs
	widths << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs24\\b Kopienzahlvarianten (CNVs)\\par}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\qj ";
	stream << "Die folgende Tabelle zeigt das Ergebnis der CNV-Analysen, die mit CNVHunter (www.github.com/imgag/ngs-bits) durchgef\\u252;hrt wurden. " << endl;
	stream << "Zur Validierung relevanter Ver\\u228;nderungen empfehlen wir eine zweite, unabh\\u228;ngige Methode. " <<endl;
	stream << "Zu jeder CNV werden die amplifizierten/deletierten Gene aus der Zielregion angegeben. Wenn ein Gen fett gedruckt ist, wurde dieses von CGI als Tumortreiber beurteilt. " << endl;
	stream << "Die restlichen Gene wurden von CGI als Passenger klassifiziert. " << endl;
	stream << "Die gesch\\u228;tzte Copy Number ist abh\\u228;ngig von Tumorgehalt und Verteilung der CNV-tragenden Zellen im Tumorgewebe." << endl;
	stream << "\\sa20\\cell\\row}" << endl << endl;

	widths.clear();
	widths = colWidths;

	QList< QList<QString> > header_cnvs;
	QList<QString> header_columns;

	header_columns << "\\qc Position" << "\\qc Gr\\u246;\\u223;e [kb]";

	if(i_cnv_type!=-1)
	{
		header_columns << "\\qc Typ";
	}

	header_columns<< "\\qc CNV" << "\\qc CN" << "\\qc Gene in dieser Region";

	//qDebug() << "CGI as planned originally";
	header_cnvs << header_columns;
	if(i_cnv_type!=-1)
	{
		RtfTools::writeRtfWholeTable(stream,header_cnvs,widths_cnv_type,20,true,true);
	}
	else
	{
		RtfTools::writeRtfWholeTable(stream,header_cnvs,widths,20,true,true);
	}

	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\qc " << "Somatische Ver\\u228;nderungen mit therapeutischer Relevanz" << "\\cell" << "\\row}" <<endl;

	widths.clear();
	widths = colWidths;

	//construct 2D QStringList to create RTF table
	QList< QList<QString> > somatic_cnv_table;
	//neccessary for filtering for target region
	NGSD db;
	//create set with target genes
	GeneSet target_genes = GeneSet::createFromFile(target_region.left(target_region.size()-4) + "_genes.txt");
	target_genes = db.genesToApproved(target_genes);

	for(int i=0;i<important_cnvs.count();i++)
	{
		CopyNumberVariant variant = important_cnvs[i];

		QList<QString> columns;
		//coordinates
		columns.append("\\fi20 " +variant.chr().str() + ":" + QString::number(variant.start()) + "-" + QString::number(variant.end()));
		//size
		columns.append("\\qr " + QString::number(round((important_cnvs[i].end()-important_cnvs[i].start())/1000.)) + "\\ri20");


		if(i_cnv_type!=-1)
		{
			columns.append("\\qc " + variant.annotations().at(i_cnv_type));
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
		//gene names, only print genes which were predicted as drivers in CGI and which lie in target region
		if(!variant.annotations().at(i_cgi_genes).isEmpty())
		{
			//cgi genes
			GeneSet cgi_genes = GeneSet::createFromText(variant.annotations().at(i_cgi_genes),',');
			//cgi_genes.insert(variant.annotations().at(i_cgi_genes).split(','));
			cgi_genes = db.genesToApproved(cgi_genes);

			QByteArrayList cgi_driver_statements = variant.annotations().at(i_cgi_driver_statement).split(',');

			QString genes_included = "";

			for(int j=0;j<cgi_genes.count();j++)
			{
				//skip if gene is not contained in target region
				if(!target_genes.contains(cgi_genes[j]))
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

				if(j <cgi_genes.count()-1)
				{
					genes_included.append(", ");
				}
			}


			columns.append(genes_included);

		}
		else
		{
			columns.append("");
		}





		somatic_cnv_table.append(columns);
	}
	widths.clear();
	if(i_cnv_type!=-1)
	{
		widths = widths_cnv_type;
	}
	else
	{
		widths = colWidths;
	}
	RtfTools::writeRtfWholeTable(stream,somatic_cnv_table,widths,20,true,false);


	//Germline
	widths.clear();
	widths << max_table_width;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\qc " << "Keimbahn" << "\\cell" << "\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell << "\\qc ";

	stream << "{\\cf2 Siehe Zusatzbefund / Keine}" << endl;

	stream << "\\cell\\row}" << endl;

	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);

	stream << begin_table_cell << "\\fs16\\qj " << "Position: Chromosomale Position (GRCh37), Gr\\u0246;\\u0223;e: Ausdehnung der CNV in Kilobasen [kb], CNV: Amplifikation (AMP) oder Deletion (DEL), CN: Copy Number, Gene in dieser Region: Gene aus der Zielregion. Tumortreiber sind fett gedruckt und Passenger normal."  << "\\cell\\row}" << endl;


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
			amplified_cnvs.insert(GeneSet::createFromText(variant.annotations().at(i_cgi_genes),','));
		}
		else
		{
			deleted_cnvs.insert(GeneSet::createFromText(variant.annotations().at(i_cgi_genes),','));
		}

	}

	widths.clear();
	widths << 2000 << max_table_width;
	qSort(deleted_cnvs.begin(),deleted_cnvs.end());
	qSort(amplified_cnvs.begin(),amplified_cnvs.end());
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\ql\\cf2 " << "Amplifizierte Gene:" << "\\cell"  <<endl;
	stream << begin_table_cell << "\\ql\\cf2 " << endl;
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
	stream << "\\par\\pard" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\ql\\cf2 " << "Deletierte Gene:" << "\\cell"  <<endl;
	stream << "\\qj\\cf2 ";

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

ReportHelper::ReportHelper()
{
}

ReportHelper::ReportHelper(QString snv_filename, GeneSet snv_germline_filter, GeneSet cnv_keep_genes_filter, QString target_region)
	: snv_filename_(snv_filename)
	, db_()
	, target_region_(target_region)
	, snv_germline_filter_(snv_germline_filter)
{
	snv_variants_.load(snv_filename_);

	QString base_name = QFileInfo(snv_filename_).baseName();
	QFileInfo(snv_filename_).dir().cdUp();

	//filename for CNV file
	cnv_filename_ = base_name;
	cnv_filename_.append("_cnvs.tsv");
	qDebug() << QFileInfo(snv_filename_).absolutePath().append("/").append(cnv_filename_);

	cnv_variants_.load(QFileInfo(snv_filename_).absolutePath().append("/").append(cnv_filename_));

	//Get genes for reimbursement (these are stored in comments section of GSvar file)
	for(int i=0;i<snv_variants_.comments().count();i++)
	{
		if(snv_variants_.comments().at(i).contains("GENES_FOR_REIMBURSEMENT"))
		{
			QByteArray genes;
			genes.append(snv_variants_.comments().at(i).split('=')[1]);

			genes_for_reimbursement_ = GeneSet::createFromText(genes,',');
			break;
		}
	}
	if(genes_for_reimbursement_.isEmpty())
	{
		genes_for_reimbursement_ = GeneSet::createFromText("");
	}


	//get cancer type which was used for CGI analysis and text for ICD10-diagnosis
	for(int i=0;i<snv_variants_.comments().count();i++)
	{
		if(snv_variants_.comments().at(i).contains("CGI_ICD10_DIAGNOSES"))
		{
			QStringList terms = snv_variants_.comments().at(i).split('=')[1].split(',');
			cgi_cancer_type_ = terms[0];
			icd10_diagnosis_text_ = terms[2];
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

	//Currently, gene filter for germline is the same for cnvs and snvs
	cnv_germline_filter_ = snv_germline_filter;
	cnv_keep_genes_filter_ = cnv_keep_genes_filter;

	//assign columns indices
	snv_index_filter_ = snv_variants_.annotationIndexByName("filter",true,true);
	snv_index_cgi_driver_statement_ = snv_variants_.annotationIndexByName("CGI_driver_statement",true,true);
	snv_index_cgi_gene_role = snv_variants_.annotationIndexByName("CGI_gene_role",true,true);
	snv_index_cgi_transcript = snv_variants_.annotationIndexByName("CGI_transcript",true,true);

	snv_index_variant_type_ = snv_variants_.annotationIndexByName("variant_type",true,true);
	snv_index_classification_ = snv_variants_.annotationIndexByName("classification",true,true);
	snv_index_gene_ = snv_variants_.annotationIndexByName("gene",true,true);

	for(int i=0;i<cnv_variants_.annotationHeaders().length();i++)
	{
		if(cnv_variants_.annotationHeaders()[i] == "CGI_genes")
		{
			cnv_index_cgi_genes_ = i;
			break;
		}
	}

	tumor_id_ = base_name.split('-')[0];
	normal_id_ = base_name.split('-')[1].split('_')[0]+'_'+base_name.split('-')[1].split('_')[1];

	//load qcml data
	QString qcml_file = base_name;
	qcml_file.append("_stats_som.qcML");
	QString qcml_file_absolute_path = QFileInfo(snv_filename).absolutePath().append("/").append(qcml_file);
	qcml_data_ = QCCollection::fromQCML(qcml_file_absolute_path);
	roi_.load(target_region);
}

VariantList ReportHelper::filterSnvForCGIDrivers()
{
	//extract obo terms for filtering
	OntologyTermCollection obo_terms(":/Resources/so-xp_2_5_3_v2.obo",true);
	QList<QByteArray> ids;
	ids << obo_terms.childIDs("SO:0001580",true); //coding variants
	ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
	OntologyTermCollection obo_terms_coding_splicing;
	foreach(const QByteArray& id, ids)
	{
		obo_terms_coding_splicing.add(obo_terms.findByID(id));
	}
	//save variants which are included in SNV report in this VariantList
	VariantList important_snvs;
	important_snvs.copyMetaData(snv_variants_);
	//Skip unimportant SNVs: CGI, filtered, OBO type
	for(int i=0;i<snv_variants_.count();i++)
	{
		Variant variant = snv_variants_[i];

		//Skip filtered variants
		if(!variant.annotations().at(snv_index_filter_).isEmpty()) continue;
		//only report variants with drug association in CGI if set, normally the case if snv_index_filter_ is also empty
		if(variant.annotations().at(snv_index_cgi_driver_statement_).isEmpty()) continue;
		//skip variants which are classified as passenger
		if(!variant.annotations().at((snv_index_cgi_driver_statement_)).contains("driver") && !variant.annotations().at((snv_index_cgi_driver_statement_)).contains("known"))
		{
			continue;
		}

		//Skip variant types which are non-coding or non-splicing by comparison with MISO terms
		QByteArrayList tmp_variant_types = variant.annotations().at(snv_index_variant_type_).split(',');

		bool skip = true;
		for(int j=0;j<tmp_variant_types.length();j++)
		{
			QByteArrayList tmp_single_variant_types =tmp_variant_types[j].split('&');
			for(int k=0;k<tmp_single_variant_types.length();k++)
			{
				//check whether variant type is in OBO terms
				if(obo_terms_coding_splicing.containsByName(tmp_single_variant_types[k]) || obo_terms_coding_splicing.containsByName(tmp_single_variant_types[k]+"_variant"))
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

VariantList ReportHelper::filterSnvForCGIPassengers()
{
	VariantList important_snvs;
	important_snvs.copyMetaData(snv_variants_);
	for(int i=0;i<snv_variants_.count();i++)
	{
		if(!snv_variants_[i].annotations().at(snv_index_filter_).isEmpty())
		{
			continue;
		}

		if(snv_variants_[i].annotations().at(snv_index_cgi_driver_statement_).contains("passenger"))
		{
			important_snvs.append(snv_variants_[i]);
		}
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
	roi_inter.load(db_.getProcessingSystem(tumor_id_, NGSD::FILE));
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

void ReportHelper::writeRtf(const QString& out_file)
{
	QString begin_table_cell = "\\pard\\intbl\\fs20 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";

	//max table widths in rtf file
	int max_table_width = 10000;
	QList<int> widths;
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(out_file);
	QTextStream stream(outfile.data());

	//stream.setCodec("ISO 8859-1");

	RtfTools::writeRtfHeader(stream);

	widths << 3000 << max_table_width;

	stream << "{\\pard\\sa270\\fs30\\b Variantenliste und technischer Bericht zum Befund vom " << QDate::currentDate().toString("dd.MM.yyyy") << "\\par}" << endl;

	stream << "{\\pard\\sa45\\fs24\\b Allgemeine Informationen:\\par}" << endl;

	//get histological tumor proportion and diagnosis from GenLab
	QString tumor_sample_name = tumor_id_.split('_')[0];
	GenLabDB db_genlab;

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
	stream << begin_table_cell << "Tumoranteil histol./molekular:\\cell" << begin_table_cell << db_genlab.tumorFraction(tumor_sample_name) << "\%";
	stream << " / "<< tumor_molecular_proportion <<"\%\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Diagnose:\\cell" << begin_table_cell << icd10_diagnosis_text_ << " (ICD10 - "<< db_genlab.diagnosis(tumor_sample_name) << ")" << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "CGI-Tumortyp:\\cell" << begin_table_cell << cgi_cancer_type_ << "\\cell\\row}" << endl;

	stream << "\\line" << endl;

	widths.clear();
	widths << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs24\\b Einzelnukleotid-Varianten (SNV) und kleine Insertionen/Deletionen (INDEL)\\par}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << "\\pard\\intbl\\fs20" << "\\qj ";
	stream << "In dieser Analyse wurden nur kodierende, nicht-synonyme Varianten sowie Splei\\u223;-Varianten ber\\u252;cksichtigt. ";
	stream << "Die ausf\\u252;hrlich gelisteten Varianten sind Biomarker, die von der CGI-Datenbank (www.cancergenomeinterpreter.org) als Tumortreiber klassifiziert wurden. ";
	stream << "Varianten, die von CGI als Tumor-Passenger eingestuft wurden, sind in einem eigenen Block aufgelistet. ";
	stream << "Details zu diesen Varianten sind auf Nachfrage erh\\u228;ltlich. Eine funktionelle Einsch\\u228;tzung der Varianten finden Sie im beigef\\u252;gten QCI-Bericht. ";
	stream << "Bitte beachten Sie, dass bei geringem Tumoranteil die Ergebnisse aufgrund niedriger Allelfrequenzen ungenauer sein k\\u246;nnen. ";
	stream << "Ver\\u228;nderungen, die in der jeweils zugeh\\u246;rigen Blutprobe gefunden wurden, k\\u246;nnen ein Hinweis auf eine erbliche Erkrankung darstellen. ";
	stream << "In diesem Fall empfehlen wir eine Vorstellung in einer humangenetischen Beratungsstelle. Weitere Informationen zur Untersuchungstechnik, ";
	stream <<  "-umfang und Auswertung finden sich in der Rubrik Qualit\\u228;tsparameter.";
	stream << "\\cell\\row}" <<endl;
	stream << "\\line" << endl;

	//SNV report
	VariantList snvs_with_cgi_annotation = filterSnvForCGIDrivers();
	VariantList further_important_snvs = filterSnvForCGIPassengers();
	widths.clear();
	//        pos     R       V       F/T     Gene    cDNA    role    evid
	widths << 2700 << 3000 << 3300 << 4500 << 5400 << 7800 << 8300 << max_table_width;
	double mutation_burden = qcml_data_.value("QC:2000053",true).asString().split(' ')[1].remove('(').toDouble();
	RtfTools::writeRtfTableSNV(stream,widths,snvs_with_cgi_annotation,further_important_snvs,mutation_burden);
	stream << "\\page" << endl;

	//CNV report
	//Prepare data for CNV report
	CnvList important_cnvs = filterCnv();

	widths.clear();
	widths << 3000 << 4100 << 5000 << 5500 << max_table_width;
	//Make rtf report
	RtfTools::writeRtfTableCNV(stream,widths,important_cnvs,target_region_);

	stream << "\\page" << endl;
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
	widths << 3500 << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs24\\b Qualit\\u228;tsparameter:\\par}" << endl;
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
	stream << begin_table_cell << "Prozessierungssystem Tumor:\\cell" << begin_table_cell <<  db_.getProcessingSystem(tumor_id_,NGSD::LONG) << "\\cell\\row}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Prozessierungssystem Normal:\\cell" << begin_table_cell << db_.getProcessingSystem(normal_id_,NGSD::LONG) << "\\cell\\row}" << endl;
	stream << "\\pard\\par" << endl;

	stream << "{\\pard\\sa45\\sb45\\fs24\\b Ensembl-Transkripte\\par}"<< endl;
	QStringList ensembl_ids;

	int i_co_sp= snvs_with_cgi_annotation.annotationIndexByName("coding_and_splicing",true,true);
	int i_cgi_transcript = snvs_with_cgi_annotation.annotationIndexByName("CGI_transcript");

	for(int i=0;i<snvs_with_cgi_annotation.count();i++)
	{
		QByteArrayList hgvs_terms = snvs_with_cgi_annotation[i].annotations().at(i_co_sp).split(',');
		QStringList tmp_ensemble_names;
		QString cgi_transcript = snvs_with_cgi_annotation[i].annotations().at(i_cgi_transcript);
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
		ensembl_ids.append(snvs_with_cgi_annotation[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[0]+"-"+snvs_with_cgi_annotation[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[1]);
	}
	for(int i=0;i<further_important_snvs.count();i++)
	{
		QByteArrayList hgvs_terms = further_important_snvs[i].annotations().at(i_co_sp).split(',');
		QStringList tmp_ensemble_names;
		QString cgi_transcript = further_important_snvs[i].annotations().at(i_cgi_transcript);
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
		ensembl_ids.append(further_important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[0]+"-"+further_important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[1]);
	}
	std::sort(ensembl_ids.begin(),ensembl_ids.end());

	ensembl_ids.removeDuplicates();

	widths.clear();
	widths << max_table_width;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\fs16 ";

	for(int i=0;i<ensembl_ids.count();i++)
	{
		stream << ensembl_ids[i];
		if(i<ensembl_ids.count()-1) stream << ", " << endl;
	}
	stream << "\\cell\\row}" << endl;

	stream << "\\pard\\par" << endl;
	//gaps
	stream <<"{\\pard\\b\\sa45\\sb45\\fs24 L\\u252;ckenstatistik:\\par}" << endl;

	widths.clear();
	widths << 2200 << max_table_width;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion:\\cell" << begin_table_cell << QFileInfo(target_region_).fileName()<<"\\cell" << "\\row}" << endl;
	GeneSet genes = GeneSet::createFromFile(target_region_.left(target_region_.size()-4) + "_genes.txt");

	GeneSet genes_for_gap_statistic = genes.intersect(genes_for_reimbursement_);

	if (!genes.isEmpty())
	{
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "Zielregion Gene (" << QString::number(genes_for_gap_statistic.count()) << "): " << "\\cell" << begin_table_cell << genes_for_gap_statistic.join(", ") << "\\cell" << "\\row}"<< endl;
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

	if(genes_for_reimbursement_.count()>0)
	{
		for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
		{
			//only print statistic for reimbursement genes
			if(!genes_for_reimbursement_.contains(it.key())) continue;

			RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
			stream << begin_table_cell << endl;
			const BedFile& gaps = it.value();
			QString chr = gaps[0].chr().strNormalized(true);
			QStringList coords;
			for (int i=0; i<gaps.count(); ++i)
			{
				coords << QString::number(gaps[i].start()) + "-" + QString::number(gaps[i].end());
			}
			stream << "\\fi20 "<< it.key() << "\\cell"<< begin_table_cell<< "\\fi20 " << gaps.baseCount() << "\\cell"<< begin_table_cell << "\\fi20 "<< chr << "\\cell"<< begin_table_cell << "\\fi20 "<<  coords.join(", ") << endl;

			stream << "\\cell" << "\\row}" << endl;
		}
	}
	else
	{
		widths.clear();
		widths << max_table_width;
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
		stream << begin_table_cell;
		stream << "\\fi20 Die L\\u252;ckenstatistik konnte nicht erstellt werden, da die GSvar-Datei keine Informationen zu abrechenbaren Genen enth\\u228;lt.";
		stream << "\\cell\\row}" << endl;
	}

	//close stream
	stream << "}";
	outfile->close();
}
