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
	stream.setCodec("ANSI");
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
			stream << "\\clbrdrt\\brdrw15\\brdrs\\clbrdrl\\brdrw15\\brdrs\\clbrdrb\\brdrw15\\brdrs\\clbrdrr\\brdrw15\\brdrs\\cellx" <<col_widths[column];
		stream << endl;
	}
	else
	{
		//Generate cells with widths out of colWidths
		for(int column=0;column<col_numbers;column++)
			stream << "\\cellx" <<col_widths[column];
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
		begin_table_cell = begin_table_cell + "\\b ";

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
	stream << "{\\pard\\sa45\\sb45\\fs24\\b SNVs und kleine INDELs:\\par}" << endl;

	//rtf somatic mutation rate
	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\qj{\\b Mutationslast: " << mutation_burden << " Varianten/Mbp}. " << endl;
	stream << "Werte von unter 3.3 Var/Mbp gelten als niedrig, Werte zwischen 3.3 Var/Mbp und unter 23.1 Var/Mbp als mittel und Werte ab ";
	stream << "23.1 Var/Mbp gelten als hoch. Die Mutationslast kann bei der Entscheidung ob eine Immuntherapie sinnvoll ist von Bedeutung sein.";
	stream << "\\cell\\row}" << endl;

	widths.clear();
	widths = colWidths;

	//rtf somatic SNV table
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\qc " << "Position\\cell" << begin_table_cell_bold << "\\qc " << "R\\cell" << begin_table_cell_bold << "\\qc " << "V\\cell" << begin_table_cell_bold;
	stream << "\\qc " << "F/T Tumor\\cell"  << begin_table_cell_bold << "\\qc " << "cDNA\\cell" << "\\row}" << endl;
	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "\\qc " << "Somatische Varianten mit therapeutischer Relevanz, die in CGI gelistet sind" << "\\cell" << "\\row}" <<endl;
	widths.clear();
	widths = colWidths;

	//construct 2D QStringList to create RTF table
	QList< QList<QString> > somatic_snv_table;
	for(int i=0;i<important_snvs.count();i++)
	{
		QList<QString> columns;
		//position
		columns.append(important_snvs[i].chr().str() + ":" + QString::number(important_snvs[i].start()) + "-" + QString::number(important_snvs[i].end()));
		//reference
		columns.append("\\qc " + important_snvs[i].ref());
		//observed
		columns.append("\\qc " + important_snvs[i].obs());
		//F/T Tumor
		columns.append("\\qc " + QString::number(important_snvs[i].annotations().at(i_tum_af).toDouble(), 'f', 3) + "/" + important_snvs[i].annotations().at(i_tum_dp));
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
				number_keep = j;
		}

		QString gene_name =  important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[0];
		QString mutation = important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[5];
		QString protein_change = important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[6];
		if(protein_change.isEmpty())
			protein_change = "p.?";
		QString ensemble_number = important_snvs[i].annotations().at(i_co_sp).split(',')[number_keep].split(':')[1];
		QString cdna = gene_name + " " + mutation + ":" + protein_change + " (" + ensemble_number + ")";
		columns.append(cdna);

		somatic_snv_table.append(columns);
	}
	RtfTools::writeRtfWholeTable(stream,somatic_snv_table,widths,20,true,false);

	widths.clear();
	widths << max_table_width;


	//rtf further genes
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell << "\\qc\\b " << "Somatische Varianten, die nicht in CGI gelistet sind:" << "\\cell" << "\\row}" <<endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell << "\\fs18\\qj ";

	QStringList gene_key;
	QMap<QString,QString> gene_code_change;
	QMap<QString,QString> gene_protein_change;
	for(int i=0;i<further_genes.count();i++)
	{
		QString gene_name = further_genes[i].annotations().at(i_gene).split(',')[0];
		QString code_change = further_genes[i].annotations().at(i_co_sp).split(',')[0].split(':')[5];
		QString protein_change = further_genes[i].annotations().at(i_co_sp).split(',')[0].split(':')[6];

		gene_key.append(gene_name);
		gene_code_change.insert(gene_name,code_change);
		gene_protein_change.insert(gene_name,protein_change);
	}
	std::sort(gene_key.begin(),gene_key.end());
	//gene_key.removeDuplicates();
	for(int i=0;i<gene_key.count();i++)
	{
		stream << gene_key[i];
		stream << " (" << gene_protein_change[gene_key[i]];
		if(gene_protein_change[gene_key[i]].isEmpty())
			stream << gene_code_change[gene_key[i]] << ":p.?";
		stream << ")";
		if(!(i >= further_genes.count()-1))
			stream << ", " << endl;
	}
	stream << "\\cell\\row}" <<endl;


	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell << "\\qc ";

	stream << "{\\cf2 Siehe Zusatzbefund / Keine}" << endl;

	stream << "\\cell\\row}" << endl;

	widths.clear();
	widths << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);

	stream << begin_table_cell << "\\fs16\\qj " << "Position: Chromosomale Position (GRCh37), R: Allel Referenz, V: Allel Variante, F/T Tumor: Frequenz/Tiefe Tumor, cDNA: cDNA Position und Auswirkung auf Peptid"  << "\\cell\\row}" << endl;
	stream << "{\\pard\\par}";

	stream << "\\line" <<endl;

}
void RtfTools::writeRtfTableCNV(QTextStream& stream, const QList<int>& colWidths, const CnvList& important_cnvs, const CnvList& further_cnvs)
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

	//get indices
	int i_cgi_drug_assoc = -1;

	for(int i=0;i<important_cnvs.annotationHeaders().count();i++)
	{
		if(important_cnvs.annotationHeaders()[i] == "CGI_drug_assoc")
			i_cgi_drug_assoc = i;
	}
	if(i_cgi_drug_assoc == -1)
	{
		THROW(FileParseException,"CNV file does not contain a CGI_drug_assoc column");
	}

	//widths if cnv_type is set in input VariantLists
	QList<int> widths_cnv_type;
	widths_cnv_type << 3000 << 4100 << 5000 << 5500 << 9000 <<max_table_width;

	//CNVs
	widths << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs24\\b CNVs:\\par}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\qj ";
	stream << "Die folgenden Tabellen zeigen das Ergebnis der CNV-Analysen, die mit CNVHunter (www.github.com/imgag/ngs-bits) durchgef\\u252;hrt wurden. " << endl;
	stream << "Zur Validierung relevanter Ver\\u228;nderungen empfehlen wir eine zweite, unabh\\u228;ngige Methode. " <<endl;
	stream << "Die gesch\\u228;tzte Copy Number ist abh\\u228;ngig von Tumorgehalt und Verteilung der CNV-tragenden Zellen im Tumorgewebe." << endl;
	stream << "\\cell\\row}" << endl << endl;

	widths.clear();
	widths = colWidths;

	QList< QList<QString> > header_cnvs;
	QList<QString> header_columns;

	header_columns << "\\qc Position" << "\\qc Gr\\u246;\\u223;e [kb]" << "\\qc Typ" << "\\qc CN" << "\\qc CGI-gelistete Gene in dieser Region";
	if(i_cnv_type!=-1)
	{
		header_columns << "\\qc Typ";
	}
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
	for(int i=0;i<important_cnvs.count();i++)
	{
		CopyNumberVariant variant = important_cnvs[i];

		QList<QString> columns;
		//coordinates
		columns.append(variant.chr().str() + ":" + QString::number(variant.start()) + "-" + QString::number(variant.end()));
		//size
		columns.append("\\qr " + QString::number(round((important_cnvs[i].end()-important_cnvs[i].start())/1000.)) );
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
			columns.append("\\qc AMP");
		else
			columns.append("\\qc DEL");
		//copy numbers
		columns.append("\\qc " + QString::number(cn));
		//gene names, only print genes in CGI
		columns.append(variant.annotations().at(i_cgi_drug_assoc));

		if(i_cnv_type!=-1)
		{
			columns.append("\\qc " + variant.annotations().at(i_cnv_type));
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

	stream << "{\\par\\pard}" << endl;


	//Amplified/Deleted Genes
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
		QVector<int>::fromList(variant.copyNumbers());

		std::sort(copy_numbers.begin(),copy_numbers.end());
		if(BasicStatistics::median(copy_numbers) > 2.)
		{
			amplified_cnvs.insert(variant.genes());
		}
		else
		{
			deleted_cnvs.insert(variant.genes());
		}
	}
	for(int i=0;i<further_cnvs.count();i++)
	{
		//calc median of cn
		CopyNumberVariant variant = further_cnvs[i];
		QVector<double> copy_numbers;
		foreach(int cn, variant.copyNumbers())
		{
			copy_numbers.append((double)cn);
		}
		std::sort(copy_numbers.begin(),copy_numbers.end());

		if(BasicStatistics::median(copy_numbers) > 2.)
		{
			amplified_cnvs.insert(variant.genes());
		}
		else
		{
			deleted_cnvs.insert(variant.genes());
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
		stream << amplified_cnvs[i];
		if(i<amplified_cnvs.count()-1)
			stream << ", ";
	}
	stream << "\\cell" << "\\row}" << endl;
	stream << "\\par\\pard" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "\\ql\\cf2 " << "Deletierte Gene:" << "\\cell"  <<endl;
	stream << "\\qj\\cf2 ";
	for(int i=0;i<deleted_cnvs.count();i++)
	{
		stream << deleted_cnvs[i];
		if(i<deleted_cnvs.count()-1)
			stream << ", ";
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
	cnv_variants_.load(QFileInfo(snv_filename_).absolutePath().append("/").append(cnv_filename_));

	//Currently, gene filter for germline is the same for cnvs and snvs
	cnv_germline_filter_ = snv_germline_filter;
	cnv_keep_genes_filter_ = cnv_keep_genes_filter;

	//assign columns indices
	snv_index_filter_ = snv_variants_.annotationIndexByName("filter",true,true);
	snv_index_cgi_drug_assoc_ = snv_variants_.annotationIndexByName("CGI_drug_assoc",true,true);
	snv_index_variant_type_ = snv_variants_.annotationIndexByName("variant_type",true,true);
	snv_index_classification_ = snv_variants_.annotationIndexByName("classification",true,true);
	snv_index_gene_ = snv_variants_.annotationIndexByName("gene",true,true);

	for(int i=0;i<cnv_variants_.annotationHeaders().length();i++)
	{
		if(cnv_variants_.annotationHeaders()[i] == "CGI_drug_assoc")
		{
			cnv_index_cgi_drug_assoc_ = i;
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

VariantList ReportHelper::filterSnvFromCgi()
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
		//only report variants with drug association in CGI if set
		if(variant.annotations().at(snv_index_cgi_drug_assoc_).isEmpty()) continue;

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

VariantList ReportHelper::filterImportantSnv()
{
	VariantList important_snvs;
	important_snvs.copyMetaData(snv_variants_);
	for(int i=0;i<snv_variants_.count();i++)
	{
		if(!snv_variants_[i].annotations().at(snv_index_filter_).isEmpty())
		{
			continue;
		}

		if(snv_variants_[i].annotations().at(snv_index_cgi_drug_assoc_).isEmpty())
		{
			important_snvs.append(snv_variants_[i]);
		}
	}

	return important_snvs;
}

VariantList ReportHelper::germlineSnv()
{
	VariantList germline_snvs;
	germline_snvs.copyMetaData(snv_variants_);

	//only keep variants which are contained in snv_germline_filter_
	VariantFilter filter_snvs(snv_variants_);
	filter_snvs.flagByGenes(snv_germline_filter_);

	for(int i=0;i<snv_variants_.count();i++)
	{
		Variant variant = snv_variants_[i];

		if(variant.annotations().at(snv_index_classification_).toInt()<4) continue;

		//only keep germline SNVs which are listed in snv_germline_filter
		if(!filter_snvs.flags()[i]) continue;

		if(variant.annotations().at(snv_index_cgi_drug_assoc_).isEmpty()) continue;
		germline_snvs.append(variant);
	}
	return germline_snvs;
}

CnvList ReportHelper::filterCnvFromCgi()
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

		//Only include CNVs with annotation in CGI
		if(!variant.annotations().at(cnv_index_cgi_drug_assoc_).isEmpty())
		{
			cgi_annotated_cnvs.append(variant);
		}
	}
	return cgi_annotated_cnvs;
}

CnvList ReportHelper::filterImportantCnv()
{
	//cgi annotated cnvs
	CnvList important_cnvs;
	important_cnvs.copyMetaData(cnv_variants_);


	for(int i=0;i<cnv_variants_.count();i++)
	{
		bool skip = false;
		CopyNumberVariant variant = cnv_variants_[i];

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

			if(cnv_keep_genes_filter_.contains(gene) && *std::max_element(zscores.begin(),zscores.end())>=5.)
			{
				skip = false;
			}
		}

		if(skip) continue;

		//Only include CNVs without annotation in CGI
		if(variant.annotations().at(cnv_index_cgi_drug_assoc_).isEmpty())
		{
			important_cnvs.append(variant);
		}
	}
	return important_cnvs;
}

CnvList ReportHelper::germlineCnv()
{
	CnvList germline_cnvs;

	germline_cnvs.copyMetaData(cnv_variants_);

	for(int i=0;i<cnv_variants_.count();i++)
	{
		bool skip = false;

		CopyNumberVariant variant = cnv_variants_[i];

		QList<double> zscores = variant.zScores();
		for(int j=0;j<zscores.length();j++)
		{
			zscores[j] = fabs(zscores[j]);
		}

		//only keep genes with high enough z-scores
		if(*std::max_element(zscores.begin(),zscores.end()) < 4.)
		{
			skip = true;
		}
		if(zscores.length()< 3.)
		{
			skip = true;
		}
		if(skip)
		{
			continue;
		}

		//only keep germline genes which are listed in cnv_germline_filter_
		skip = true;
		foreach (QByteArray gene, variant.genes())
		{
			if(cnv_germline_filter_.contains(gene))
				skip = false;
		}

		if(skip)
		{
			continue;
		}
		germline_cnvs.append(variant);
	}
	return germline_cnvs;
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

void ReportHelper::writeRtf(QString out_file)
{
	QString begin_table_cell = "\\pard\\intbl\\fs20 ";
	QString begin_table_cell_bold = begin_table_cell+"\\b ";

	//max table widths in rtf file
	int max_table_width = 10000;
	QList<int> widths;
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(out_file);
	QTextStream stream(outfile.data());
	RtfTools::writeRtfHeader(stream);
	stream.setCodec("UTF-8");

	widths << 3000 << max_table_width;

	stream << "{\\pard\\sa270\\fs30\\b Variantenliste und technischer Bericht zum Befund vom " << QDate::currentDate().toString("dd.MM.yyyy") << "\\par}" << endl;

	stream << "{\\pard\\sa45\\fs24\\b Allgemeine Informationen:\\par}" << endl;

	//get histological tumor proportion and diagnosis from GenLab
	QString tumor_sample_name = tumor_id_.split('_')[0];
	GenLabDB db_genlab;

	double tumor_molecular_proportion = qcml_data_.value("QC:2000054",true).toString().toDouble();
	if(tumor_molecular_proportion > 100.)
		tumor_molecular_proportion = 100.;

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
	stream << begin_table_cell << "Diagnose:\\cell" << begin_table_cell << db_genlab.diagnosis(tumor_sample_name) <<" {\\cf2 TODO}" << "\\cell\\row}" << endl;

	stream << "\\line" << endl;

	widths.clear();
	widths << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs24\\b Varianten:\\par}" << endl;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << "\\pard\\intbl\\fs20" << "\\qj ";
	stream << "In dieser Analyse wurden nur kodierende, nicht-synonyme Varianten sowie Splei\\u223;-Varianten ber\\u252;cksichtigt. ";
	stream << "Die ausf\\u252;hrlich gelisteten Varianten sind Biomarker, die in der CGI-Datenbank (www.cancergenomeinterpreter.org) gelistet werden. ";
	stream << "Die verbleibenden Gene mit Varianten werden im Block 'weitere Gene mit Varianten' zusammengefasst. ";
	stream << "Details zu diesen Varianten sind auf Nachfrage erh\\u228;ltlich. Eine funktionelle Einsch\\u228;tzung der Varianten finden Sie im beigef\\u252;gten QCI-Bericht. ";
	stream << "Bitte beachten Sie, dass bei geringem Tumoranteil die Ergebnisse aufgrund niedriger Allelfrequenzen ungenauer sein k\\u246;nnen. ";
	stream << "Ver\\u228;nderungen, die in der jeweils zugeh\\u246;rigen Blutprobe gefunden wurden, k\\u246;nnen ein Hinweis auf eine erbliche Erkrankung darstellen. ";
	stream << "In diesem Fall empfehlen wir eine Vorstellung in einer humangenetischen Beratungsstelle. Weitere Informationen zur Untersuchungstechnik, ";
	stream <<  "-umfang und Auswertung finden sich in der Rubrik Qualit\\u228;tsparameter.";
	stream << "\\cell\\row}" <<endl;
	stream << "\\line" << endl;

	//SNV report
	VariantList cgi_annotated_snvs = filterSnvFromCgi();
	VariantList further_important_snvs = filterImportantSnv();
	widths.clear();
	widths << 2500 << 3000 <<3500 << 4750 << max_table_width;
	double mutation_burden = qcml_data_.value("QC:2000053",true).asString().split(' ')[1].remove('(').toDouble();
	RtfTools::writeRtfTableSNV(stream,widths,cgi_annotated_snvs,further_important_snvs,mutation_burden);
	stream << "\\page" << endl;

	//CNV report
	//Prepare data for CNV report
	CnvList cgi_annotated_cnvs = filterCnvFromCgi();
	CnvList further_important_cnvs = filterImportantCnv();

	widths.clear();
	widths << 3000 << 4100 << 5000 << 5500 << max_table_width;
	//Make rtf report
	RtfTools::writeRtfTableCNV(stream,widths,cgi_annotated_cnvs,further_important_cnvs);

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
	widths << 4000 << max_table_width;
	stream << "{\\pard\\sa45\\sb45\\fs24\\b Qualit\\u228;sparameter:\\par}" << endl;
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
	stream <<"{\\pard\\b\\sa45\\sb45\\fs24 L\\u252;ckenstatistik:\\par}" << endl;

	widths.clear();
	widths << 2000 << max_table_width;

	RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
	stream << begin_table_cell << "Zielregion:\\cell" << begin_table_cell << QFileInfo(target_region_).fileName()<<"\\cell" << "\\row}" << endl;
	GeneSet genes = GeneSet::createFromFile(target_region_.left(target_region_.size()-4) + "_genes.txt");
	if (!genes.isEmpty())
	{
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,false);
		stream << begin_table_cell << "Zielregion Gene (" << QString::number(genes.count()) << "): " << "\\cell" << begin_table_cell << genes.join(", ") << "\\cell" << "\\row}"<< endl;
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
	stream << begin_table_cell << "L\\u252;cken Basen:\\cell" << begin_table_cell << low_cov.baseCount() << " (" << QString::number(100.0 * low_cov.baseCount()/roi_.baseCount(), 'f', 2) << "%)"<< "\\cell" << "\\row}" <<endl;

	QHash<QByteArray, BedFile> grouped = gapStatistics();
	widths.clear();
	widths << 900 << 1800 << 3000 << max_table_width;
	RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
	stream << begin_table_cell_bold << "Gen\\cell" << begin_table_cell_bold << "L\\u252;cken\\cell" << begin_table_cell_bold << "Chromosom\\cell" << begin_table_cell_bold << "Koordinaten (GRCh37)" << "\\cell" <<"\\row}" << endl;

	for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
	{
		RtfTools::writeRtfTableSingleRowSpec(stream,widths,true);
		stream << begin_table_cell << endl;
		const BedFile& gaps = it.value();
		QString chr = gaps[0].chr().strNormalized(true);
		QStringList coords;
		for (int i=0; i<gaps.count(); ++i)
		{
			coords << QString::number(gaps[i].start()) + "-" + QString::number(gaps[i].end());
		}
		stream << it.key() << "\\cell"<< begin_table_cell << gaps.baseCount() << "\\cell"<< begin_table_cell << chr << "\\cell"<< begin_table_cell << coords.join(", ") << endl;

		stream << "\\cell" << "\\row}" << endl;
	}

	//close stream
	stream << "}";
	outfile->close();
}
