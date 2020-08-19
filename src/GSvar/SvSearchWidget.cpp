#include "SvSearchWidget.h"
#include "Exceptions.h"
#include "Chromosome.h"
#include "Helper.h"
#include "FilterCascade.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QClipboard>

SvSearchWidget::SvSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
	, db_()
{
	ui_.setupUi(this);
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(search()));

	connect(ui_.rb_single_sv, SIGNAL(clicked(bool)), this, SLOT(changeSearchType()));
	connect(ui_.rb_region, SIGNAL(clicked(bool)), this, SLOT(changeSearchType()));
	connect(ui_.rb_genes, SIGNAL(clicked(bool)), this, SLOT(changeSearchType()));
}

void SvSearchWidget::setCoordinates(const BedpeLine& sv_coordinates)
{
	ui_.coordinates1->setText(sv_coordinates.chr1().strNormalized(true) + ":" + QString::number(sv_coordinates.start1()) + "-" + QString::number(sv_coordinates.end1()));
	ui_.coordinates2->setText(sv_coordinates.chr2().strNormalized(true) + ":" + QString::number(sv_coordinates.start2()) + "-" + QString::number(sv_coordinates.end2()));
	ui_.svType->setCurrentText(BedpeFile::typeToString(sv_coordinates.type()));
}

void SvSearchWidget::setProcessedSampleId(QString ps_id)
{
	ps_id_ = ps_id;
	ui_.same_processing_system_only->setEnabled((ps_id_ != ""));
	ui_.same_processing_system_only->setChecked(ui_.same_processing_system_only->isEnabled());
}

void SvSearchWidget::search()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	// clear table
	ui_.table->clear();
	ui_.table->setColumnCount(0);

	try
	{
		// SV type/table
		StructuralVariantType type = BedpeFile::stringToType(ui_.svType->currentText().toUtf8());
		QString sv_table = db_.svTableName(type) + " as sv ";

		// define table columns
		QByteArrayList selected_columns;
		selected_columns << "sv.id" << "CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as sample" << "ps.quality as quality_sample "
						 << "sys.name_manufacturer as system" << "s.disease_group" << "s.disease_status"  << "sc.caller" << "sc.caller_version"
						 << "\"" + BedpeFile::typeToString(type) + "\" AS type";

		// define type specific table columns
		if(type==StructuralVariantType::BND) selected_columns << "sv.chr1" << "sv.start1" << "sv.end1" << "sv.chr2" << "sv.start2" << "sv.end2";
		else if(type==StructuralVariantType::INS) selected_columns << "sv.chr" << "sv.pos AS start" << "(sv.pos + sv.ci_upper) AS end";
		else selected_columns << "sv.chr" << "sv.start_min" << "sv.start_max" << "sv.end_min" << "sv.end_max";
		selected_columns << "rpc.class";

		// query part for position match
		QByteArray query_same_position;

		if (ui_.rb_single_sv->isChecked())
		{
			//(0) parse input
			QStringList coords1 = ui_.coordinates1->text().replace("-", " ").replace(":", " ").replace(",", "").split(QRegularExpression("\\W+"), QString::SkipEmptyParts);
			if (coords1.count()!=3) THROW(ArgumentException, "Could not split first coordinates in three parts! " + QString::number(coords1.count()) + " parts found.");
			QStringList coords2 = ui_.coordinates2->text().replace("-", " ").replace(":", " ").replace(",", "").split(QRegularExpression("\\W+"), QString::SkipEmptyParts);
			if (coords2.count()!=3) THROW(ArgumentException, "Could not split second coordinates in three parts! " + QString::number(coords2.count()) + " parts found.");

			Chromosome chr1(coords1[0]);
			if (!chr1.isValid()) THROW(ArgumentException, "Invalid first chromosome given: " + coords1[0]);
			int start1 = Helper::toInt(coords1[1], "First start cooridinate");
			int end1 = Helper::toInt(coords1[2], "First end cooridinate");

			Chromosome chr2(coords2[0]);
			if (!chr2.isValid()) THROW(ArgumentException, "Invalid second chromosome given: " + coords2[0]);
			int start2 = Helper::toInt(coords2[1], "Second start cooridinate");
			int end2 = Helper::toInt(coords2[2], "Second end cooridinate");

			bool perform_exact_match = (ui_.operation->currentText() == "exact match");


			//(1) validate input
			if (!chr1.isValid()) THROW(ArgumentException, "Invalid first chromosome given: " + coords1[0]);
			if (!chr2.isValid()) THROW(ArgumentException, "Invalid second chromosome given: " + coords2[0]);
			if(type!=StructuralVariantType::BND && (chr1 != chr2))
			{
				THROW(ArgumentException, "Invalid SV position: " + BedpeFile::typeToString(type) + " " + ui_.coordinates1->text() + " " + ui_.coordinates2->text());
			}

			if(!chr1.isNonSpecial() || !chr2.isNonSpecial())
			{
				THROW(ArgumentException,"SVs on special chromosomes are not supported by the NGSD!" );
			}

			//(2) define SQL query for position
			// query for SV position
			if(type==StructuralVariantType::BND)
			{
				query_same_position += "sv.chr1 = \"" + chr1.strNormalized(true) + "\" AND sv.start1 <= " + QByteArray::number(end1) + " AND ";
				query_same_position += QByteArray::number(start1) + " <= sv.end1 AND sv.chr2 = \"" + chr2.strNormalized(true) + "\" AND sv.start2 <= ";
				query_same_position +=  QByteArray::number(end2) + " AND " + QByteArray::number(start2) + " <= sv.end2 ";
			}
			else if(type==StructuralVariantType::INS)
			{
				int min_pos = std::min(start1, start2);
				int max_pos = std::max(end1, end2);
				query_same_position += "sv.chr = \"" + chr1.strNormalized(true) + "\" AND sv.pos <= " + QByteArray::number(max_pos) + " AND ";
				query_same_position += QByteArray::number(min_pos) + " <= (sv.pos + sv.ci_upper) ";
			}
			else
			{
				//DEL, DUP or INV
				if(perform_exact_match)
				{
					query_same_position += "sv.chr = \"" + chr1.strNormalized(true) + "\" AND sv.start_min <= " + QByteArray::number(end1) + " AND ";
					query_same_position += QByteArray::number(start1) + " <= sv.start_max AND sv.end_min <= " + QByteArray::number(end2) + " AND ";
					query_same_position += QByteArray::number(start2) + " <= sv.end_max ";
				}
				else
				{
					//overlap
					query_same_position += "sv.chr = \"" + chr1.strNormalized(true) + "\" AND sv.start_min <= " + QByteArray::number(end2) + " AND ";
					query_same_position += QByteArray::number(start1) + " <= sv.end_max ";
				}
			}
		}
		else if (ui_.rb_region->isChecked())
		{
			// (0) parse input
			QStringList coords = ui_.le_region->text().replace("-", " ").replace(":", " ").replace(",", "").split(QRegularExpression("\\W+"), QString::SkipEmptyParts);
			if (coords.count()!=3) THROW(ArgumentException, "Could not split region coordinates in three parts! " + QString::number(coords.count()) + " parts found.");

			Chromosome chr(coords[0]);
			if (!chr.isValid()) THROW(ArgumentException, "Invalid chromosome given: " + coords[0]);
			int start = Helper::toInt(coords[1], "Region start cooridinate");
			int end = Helper::toInt(coords[2], "Region end cooridinate");


			//(1) validate input
			if (!chr.isValid()) THROW(ArgumentException, "Invalid chromosome given: " + coords[0]);
			if(!chr.isNonSpecial())	THROW(ArgumentException,"SVs on special chromosomes are not supported by the NGSD!" );


			//(2) define SQL query for position
			if(type==StructuralVariantType::BND)
			{
				query_same_position += "(( sv.chr1 = \"" + chr.strNormalized(true) + "\" AND sv.start1 <= " + QByteArray::number(end) + " AND ";
				query_same_position += QByteArray::number(start) + " <= sv.end1 ) OR ( sv.chr2 = \"" + chr.strNormalized(true) + "\" AND sv.start2 <= ";
				query_same_position +=  QByteArray::number(end) + " AND " + QByteArray::number(start) + " <= sv.end2 )) ";
			}
			else if(type==StructuralVariantType::INS)
			{
				query_same_position += "sv.chr = \"" + chr.strNormalized(true) + "\" AND sv.pos <= " + QByteArray::number(end) + " AND ";
				query_same_position += QByteArray::number(start) + " <= (sv.pos + sv.ci_upper) ";
			}
			else
			{
				//DEL, DUP or INV
				query_same_position += "sv.chr = \"" + chr.strNormalized(true) + "\" AND sv.start_min <= " + QByteArray::number(end) + " AND ";
				query_same_position += QByteArray::number(start) + " <= sv.end_max ";
			}
		}
		else // ui_.rb_genes->isChecked()
		{
			// (0) + (1) parse input and validate
			GeneSet genes;
			foreach (const QString& gene, ui_.le_genes->text().replace(";", " ").replace(",", "").split(QRegularExpression("\\W+"), QString::SkipEmptyParts))
			{
				QByteArray approved_gene_name = db_.geneToApproved(gene.toUtf8());
				if (approved_gene_name == "") THROW(ArgumentException, "Invalid gene name '" + gene + "' given!");
				genes.insert(approved_gene_name);
			}
			if (genes.count() == 0) THROW(ArgumentException, "No valid gene names provided!");

			// convert GeneSet to region
			BedFile region = db_.genesToRegions(genes, Transcript::ENSEMBL, "gene");
			region.extend(5000);
			region.sort();
			region.merge();


			//(2) define SQL query for position
			QByteArrayList query_pos_overlap;
			if(type==StructuralVariantType::BND)
			{
				for (int i = 0; i < region.count(); ++i)
				{
					QByteArray query_single_region;
					query_single_region += "(( sv.chr1 = \"" + region[i].chr().strNormalized(true) + "\" AND sv.start1 <= " + QByteArray::number(region[i].end()) + " AND ";
					query_single_region += QByteArray::number(region[i].start()) + " <= sv.end1 ) OR ( sv.chr2 = \"" + region[i].chr().strNormalized(true) + "\" AND sv.start2 <= ";
					query_single_region +=  QByteArray::number(region[i].end()) + " AND " + QByteArray::number(region[i].start()) + " <= sv.end2 )) ";
					query_pos_overlap.append(query_single_region);
				}
			}
			else if(type==StructuralVariantType::INS)
			{
				for (int i = 0; i < region.count(); ++i)
				{
					QByteArray query_single_region;
					query_single_region += "(sv.chr = \"" + region[i].chr().strNormalized(true) + "\" AND sv.pos <= " + QByteArray::number(region[i].end()) + " AND ";
					query_single_region += QByteArray::number(region[i].start()) + " <= (sv.pos + sv.ci_upper)) ";
					query_pos_overlap.append(query_single_region);
				}
			}
			else
			{
				//DEL, DUP or INV
				for (int i = 0; i < region.count(); ++i)
				{
					QByteArray query_single_region;
					query_single_region += "(sv.chr = \"" + region[i].chr().strNormalized(true) + "\" AND sv.start_min <= " + QByteArray::number(region[i].end()) + " AND ";
					query_single_region += QByteArray::number(region[i].start()) + " <= sv.end_max) ";
					query_pos_overlap.append(query_single_region);
				}
			}

			//concatinate single pos querie
			query_same_position += "(" + query_pos_overlap.join("OR ") + ") ";
		}


		//(3) define SQL queries for filters

		// filter by processing system
		QString query_same_ps;
		// get processing system id
		if (ps_id_ != "" && ui_.same_processing_system_only->isChecked())
		{
			int processing_system_id = db_.processingSystemIdFromProcessedSample(db_.processedSampleName(ps_id_));
			query_same_ps = "ps.processing_system_id = " + QByteArray::number(processing_system_id) + " ";
		}

		// filter by quality
		QStringList allowed_qualities;
		if(ui_.q_ps_bad->isChecked()) allowed_qualities << "\"bad\"";
		if(ui_.q_ps_medium->isChecked()) allowed_qualities << "\"medium\"";
		if(ui_.q_ps_good->isChecked()) allowed_qualities << "\"good\"";
		if(ui_.q_ps_na->isChecked()) allowed_qualities << "\"n/a\"";

		//skip db query if no quality is selected
		if(allowed_qualities.size() == 0)
		{
			THROW(ArgumentException,"No quality selected!" );
		}

		QString query_ps_quality;
		// add filter if not all check boxes are selected
		if(allowed_qualities.size() != 4) query_ps_quality = "ps.quality IN (" + allowed_qualities.join(", ") + ") ";

		// combine all conditions
		QStringList conditions;
		if(!query_same_ps.isEmpty()) conditions << query_same_ps;
		if(!query_ps_quality.isEmpty()) conditions << query_ps_quality;
		conditions << query_same_position;


		//(4) create SQL table
		QString query_join = "SELECT " + selected_columns.join(", ") + " FROM " + sv_table
				+ "INNER JOIN sv_callset sc ON sv.sv_callset_id = sc.id "
				+ "INNER JOIN processed_sample ps ON sc.processed_sample_id = ps.id "
				+ "INNER JOIN sample s ON ps.sample_id = s.id "
				+ "INNER JOIN processing_system sys ON ps.processing_system_id = sys.id "
				+ "LEFT JOIN report_configuration_sv rpc ON sv.id = rpc." + sv_table.split(" ")[0] +"_id "
				+ "WHERE " + conditions.join(" AND ")
				+ "ORDER BY ps.id ";

		DBTable table = db_.createTable("sv", query_join);


		//(5) show samples with SVs in table
		ui_.table->setData(table);
		ui_.message->setText("Found " + QString::number(ui_.table->rowCount()) + " matching SVs in NGSD.");


	}
	catch(Exception& e)
	{
		ui_.message->setText("Error: Search could not be performed:\t" + e.message());
		QMessageBox::warning(this, "SV search", "Error: Search could not be performed:\n" + e.message());
	}

	QApplication::restoreOverrideCursor();
}

void SvSearchWidget::delayedInitialization()
{
	if (ui_.coordinates1->text().trimmed()!="" && ui_.coordinates2->text().trimmed()!="")
	{
		search();
	}
}

void SvSearchWidget::changeSearchType()
{
	ui_.coordinates1->setEnabled(ui_.rb_single_sv->isChecked());
	ui_.coordinates2->setEnabled(ui_.rb_single_sv->isChecked());
	ui_.operation->setEnabled(ui_.rb_single_sv->isChecked());
	ui_.le_region->setEnabled(ui_.rb_region->isChecked());
	ui_.le_genes->setEnabled(ui_.rb_genes->isChecked());
}
