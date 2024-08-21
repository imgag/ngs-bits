#include "SvSearchWidget.h"
#include "Exceptions.h"
#include "Chromosome.h"
#include "Helper.h"
#include "FilterCascade.h"
#include "NGSHelper.h"
#include "GlobalServiceProvider.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QClipboard>
#include <QAction>

SvSearchWidget::SvSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, db_()
{
	ui_.setupUi(this);
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(search()));

	connect(ui_.rb_single_sv->group(), SIGNAL(buttonToggled(int,bool)), this, SLOT(changeSearchType()));

	QAction* action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openSelectedSampleTabs()));
}

void SvSearchWidget::setVariant(const BedpeLine& sv)
{
	//type
	ui_.type->setCurrentText(BedpeFile::typeToString(sv.type()));

	//coordinates
	ui_.coordinates1->setText(sv.chr1().strNormalized(true) + ":" + QString::number(sv.start1()) + "-" + QString::number(sv.end1()));
	ui_.coordinates2->setText(sv.chr2().strNormalized(true) + ":" + QString::number(sv.start2()) + "-" + QString::number(sv.end2()));

	ui_.rb_single_sv->setChecked(true);
}

void SvSearchWidget::setProcessedSampleId(QString ps_id)
{
	ps_id_ = ps_id;

	ui_.same_processing_system_only->setEnabled(ps_id_!="");
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
		//not for restricted users
		LoginManager::checkRoleNotIn(QStringList{"user_restricted"});

		// SV type/table
		StructuralVariantType type = BedpeFile::stringToType(ui_.type->currentText().toUtf8());
		QString sv_table = db_.svTableName(type);

		// define table columns
		QByteArrayList selected_columns;
		selected_columns << "sv.id" << "CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as sample" << "s.gender" << "ps.quality as quality_sample "
						 << "sys.name_manufacturer as system" << "s.disease_group" << "s.disease_status" << "s.id as 'HPO terms'" << "ds.outcome" << "sc.caller" << "sc.caller_version"
						 << "\"" + BedpeFile::typeToString(type) + "\" AS type" << "sv.genotype";

		// define type specific table columns
		if(type==StructuralVariantType::BND) selected_columns << "sv.chr1" << "sv.start1" << "sv.end1" << "sv.chr2" << "sv.start2" << "sv.end2";
		else if(type==StructuralVariantType::INS) selected_columns << "sv.chr" << "sv.pos AS start" << "(sv.pos + sv.ci_upper) AS end";
		else selected_columns << "sv.chr" << "sv.start_min" << "sv.start_max" << "sv.end_min" << "sv.end_max";
		selected_columns << "rcs.class" << "CONCAT(rcs.comments, ' // ', rcs.comments2) as report_config_comments";

		// query part for position match
		QByteArray query_same_position;

		if (ui_.rb_single_sv->isChecked())
		{
			//(0) parse input

			Chromosome chr1;
			int start1, end1;
			NGSHelper::parseRegion(ui_.coordinates1->text(), chr1, start1, end1);

			Chromosome chr2;
			int start2, end2;
			NGSHelper::parseRegion(ui_.coordinates2->text(), chr2, start2, end2);

			//(1) validate input
			if (!chr1.isValid()) THROW(ArgumentException, "Invalid first chromosome given in: " + ui_.coordinates1->text());
			if (!chr2.isValid()) THROW(ArgumentException, "Invalid second chromosome given in: " + ui_.coordinates2->text());
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
				bool perform_exact_match = (ui_.operation->currentText() == "exact match");
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
			Chromosome chr;
			int start, end;
			NGSHelper::parseRegion(ui_.le_region->text(), chr, start, end);
			if(!chr.isNonSpecial())	THROW(ArgumentException, "SVs on special chromosomes are not supported by the NGSD!");

			//(1) define SQL query for position
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

			//concatinate single pos query
			query_same_position += "(" + query_pos_overlap.join("OR ") + ") ";
		}
		QStringList conditions;
		conditions << query_same_position;

		// filter by processing system
		// get processing system id
		if (ps_id_ != "" && ui_.same_processing_system_only->isChecked())
		{
			int processing_system_id = db_.processingSystemIdFromProcessedSample(db_.processedSampleName(ps_id_));
			conditions << "ps.processing_system_id = " + QByteArray::number(processing_system_id) + " ";
		}

		// filter by processed sample quality
		QStringList allowed_qualities;
		if(ui_.q_ps_bad->isChecked()) allowed_qualities << "\"bad\"";
		if(ui_.q_ps_medium->isChecked()) allowed_qualities << "\"medium\"";
		if(ui_.q_ps_good->isChecked()) allowed_qualities << "\"good\"";
		if(ui_.q_ps_na->isChecked()) allowed_qualities << "\"n/a\"";
		if(allowed_qualities.size() != 4)
		{
			conditions << "ps.quality IN (" + allowed_qualities.join(", ") + ") ";
		}


		// filter by project type
		QStringList project_types;
		if(ui_.p_diagnostic->isChecked()) project_types << "\"diagnostic\"";
		if(ui_.p_research->isChecked()) project_types << "\"research\"";
		if(ui_.p_external->isChecked()) project_types << "\"external\"";
		if(ui_.p_test->isChecked()) project_types << "\"test\"";
		if(project_types.size() != 4)
		{
			conditions << "p.type IN (" + project_types.join(", ") + ") ";
		}

		//create SQL table
		QString query_join = "SELECT " + selected_columns.join(", ") + " FROM " + sv_table + " as sv "
				+ "INNER JOIN sv_callset sc ON sv.sv_callset_id = sc.id "
				+ "INNER JOIN processed_sample ps ON sc.processed_sample_id = ps.id "
				+ "INNER JOIN sample s ON ps.sample_id = s.id "
				+ "INNER JOIN processing_system sys ON ps.processing_system_id = sys.id "
				+ "INNER JOIN project p ON ps.project_id = p.id "
				+ "LEFT JOIN report_configuration_sv rcs ON sv.id = rcs." + sv_table +"_id "
				+ "LEFT JOIN diag_status ds ON sc.processed_sample_id=ds.processed_sample_id "
				+ "WHERE " + conditions.join(" AND ")
				+ "ORDER BY ps.id ";

		DBTable table = db_.createTable("sv", query_join);

		//add size
		if (type==StructuralVariantType::DEL || type==StructuralVariantType::DUP || type==StructuralVariantType::INV)
		{
			int c_start = table.columnIndex("start_min");
			int c_end = table.columnIndex("end_max");

			QStringList sizes;
			for (int r=0; r<table.rowCount(); ++r)
			{
				double size_kb = (table.row(r).value(c_end).toDouble() - table.row(r).value(c_start).toDouble()) / 1000.0;
				sizes << QString::number(size_kb, 'f', 3);
			}
			table.insertColumn(table.columnIndex("end_max")+1, sizes, "size (kb)");
		}

		//determine HPO terms
		int hpo_col_index = table.columnIndex("HPO terms");
		QStringList sample_ids = table.extractColumn(hpo_col_index);
		QStringList hpo_terms;
		foreach(const QString& sample_id, sample_ids)
		{
			hpo_terms << db_.samplePhenotypes(sample_id).toString();
		}
		table.setColumn(hpo_col_index, hpo_terms);

		//add validation information
		QStringList validation_data;
		for (int r=0; r<table.rowCount(); ++r)
		{
			const DBRow& row = table.row(r);
			QString sv_id = row.id();
			QString s_id = db_.sampleId(row.value(0));
			validation_data << db_.getValue("SELECT status FROM variant_validation WHERE sample_id=" + s_id + " AND " + sv_table +"_id=" + sv_id).toString();
		}
		table.addColumn(validation_data, "validation_information");

		//filter by size
		if (ui_.type->currentText()=="DEL" || ui_.type->currentText()=="DUP" || ui_.type->currentText()=="INV")
		{
			int c_size = table.columnIndex("size (kb)");

			QString min_kb_str = ui_.size_min_kb->text().trimmed();
			if (!min_kb_str.isEmpty())
			{
				double min_kb = min_kb_str.isEmpty() ? -1 : Helper::toDouble(min_kb_str, "minimum size");
				for (int r=table.rowCount()-1; r>=0; --r)
				{
					const QString& size_str = table.row(r).value(c_size);
					if (!size_str.isEmpty())
					{
						double size_kb = Helper::toDouble(size_str, "size");
						if (size_kb < min_kb)
						{
							table.removeRow(r);
						}
					}
				}
			}

			QString max_kb_str = ui_.size_max_kb->text().trimmed();
			if (!max_kb_str.isEmpty())
			{
				double max_kb = max_kb_str.isEmpty() ? -1 : Helper::toDouble(max_kb_str, "maximum size");
				for (int r=table.rowCount()-1; r>=0; --r)
				{
					const QString& size_str = table.row(r).value(c_size);
					if (!size_str.isEmpty())
					{
						double size_kb = Helper::toDouble(size_str, "minimum size");
						if (size_kb > max_kb)
						{
							table.removeRow(r);
						}
					}
				}
			}
		}

		//show samples with SVs in table
		ui_.table->setData(table);
		ui_.table->showTextAsTooltip("report_config_comments");
		ui_.message->setText("Found " + QString::number(ui_.table->rowCount()) + " matching SVs in NGSD.");

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "SV search could not be performed");
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

void SvSearchWidget::openSelectedSampleTabs()
{
	int col = ui_.table->columnIndex("sample");
	foreach (int row, ui_.table->selectedRows().toList())
	{
		QString ps = ui_.table->item(row, col)->text();
		GlobalServiceProvider::openProcessedSampleTab(ps);
	}
}
