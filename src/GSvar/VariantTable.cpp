#include "VariantTable.h"
#include "GUIHelper.h"
#include "Exceptions.h"

#include <QBitArray>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>

VariantTable::VariantTable(QWidget* parent)
	: QTableWidget(parent)
{
}

void VariantTable::update(const VariantList variants_, const FilterResult& filter_result_, const GeneSet& imprinting_genes_, int max_variants)
{
	int row_count_new = std::min(filter_result_.countPassing(), max_variants);
	int col_count_new = 5 + variants_.annotations().count();
	if (rowCount()!=row_count_new || columnCount()!=col_count_new)
	{
		//completely clear items (is faster then resizing)
		clearContents();
		//set new size
		setRowCount(row_count_new);
		setColumnCount(col_count_new);
	}

	//header
	setHorizontalHeaderItem(0, createTableItem("chr"));
	horizontalHeaderItem(0)->setToolTip("Chromosome of variant");
	setHorizontalHeaderItem(1, createTableItem("start"));
	horizontalHeaderItem(1)->setToolTip("Genomic start position of variant");
	setHorizontalHeaderItem(2, createTableItem("end"));
	horizontalHeaderItem(2)->setToolTip("Genomic end position of variant");
	setHorizontalHeaderItem(3, createTableItem("ref"));
	horizontalHeaderItem(3)->setToolTip("Reference genome sequence");
	setHorizontalHeaderItem(4, createTableItem("obs"));
	horizontalHeaderItem(4)->setToolTip("Sequence observed in the sample");
	for (int i=0; i<variants_.annotations().count(); ++i)
	{
		QString anno = variants_.annotations()[i].name();
		QTableWidgetItem* header = new QTableWidgetItem(anno);

		//additional descriptions for filter column
		QString add_desc = "";
		if (anno=="filter")
		{
			auto it = variants_.filters().cbegin();
			while (it!=variants_.filters().cend())
			{
				add_desc += "\n - "+it.key() + ": " + it.value();
				++it;
			}
		}

		//additional descriptions and color for genotype columns
		SampleHeaderInfo sample_data = variants_.getSampleHeader(false);
		foreach(const SampleInfo& info, sample_data)
		{
			if (info.column_name==anno)
			{
				auto it = info.properties.cbegin();
				while(it != info.properties.cend())
				{
					add_desc += "\n - " + it.key() + ": " + it.value();

					if (info.isAffected())
					{
						header->setForeground(QBrush(Qt::darkRed));
					}

					++it;
				}
			}
		}

		QString header_desc = variants_.annotationDescriptionByName(anno, false, false).description();
		header->setToolTip(header_desc + add_desc);
		setHorizontalHeaderItem(i+5, header);
	}

	//content
	int i_genes = variants_.annotationIndexByName("gene", true, false);
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, false);
	int i_validation = variants_.annotationIndexByName("validation", true, false);
	int i_classification = variants_.annotationIndexByName("classification", true, false);
	int i_comment = variants_.annotationIndexByName("comment", true, false);
	int i_ihdb_hom = variants_.annotationIndexByName("NGSD_hom", true, false);
	int i_ihdb_het = variants_.annotationIndexByName("NGSD_het", true, false);
	int i_clinvar = variants_.annotationIndexByName("ClinVar", true, false);
	int i_hgmd = variants_.annotationIndexByName("HGMD", true, false);
	int r = -1;
	for (int i=0; i<variants_.count(); ++i)
	{
		if (!filter_result_.passing(i)) continue;

		++r;
		if (r>=max_variants) break; //maximum number of variants reached > abort
		const Variant& variant = variants_[i];

		setItem(r, 0, createTableItem(variant.chr().str()));
		if (!variant.chr().isAutosome())
		{
			item(r,0)->setBackgroundColor(Qt::yellow);
			item(r,0)->setToolTip("Not autosome");
		}
		setItem(r, 1, createTableItem(QByteArray::number(variant.start())));
		setItem(r, 2, createTableItem(QByteArray::number(variant.end())));
		setItem(r, 3, createTableItem(variant.ref()));
		setItem(r, 4, createTableItem(variant.obs()));
		bool is_warning_line = false;
		bool is_notice_line = false;
		bool is_ok_line = false;
		for (int j=0; j<variant.annotations().count(); ++j)
		{
			const QByteArray& anno = variant.annotations().at(j);
			QTableWidgetItem* item = createTableItem(anno);

			//warning
			if (j==i_co_sp && anno.contains(":HIGH:"))
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}
			else if (j==i_classification && (anno=="3" || anno=="M"))
			{
				item->setBackgroundColor(QColor(255, 135, 60)); //orange
				is_notice_line = true;
			}
			else if (j==i_classification && (anno=="4" || anno=="5"))
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}
			else if (j==i_clinvar && anno.contains("pathogenic")) //matches "pathogenic" and "likely pathogenic"
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}
			else if (j==i_hgmd && anno.contains("CLASS=DM")) //matches both "DM" and "DM?"
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}

			//non-pathogenic
			if (j==i_classification && (anno=="0" || anno=="1" || anno=="2"))
			{
				item->setBackgroundColor(Qt::green);
				is_ok_line = true;
			}

			//highlighed
			if (j==i_validation && anno.contains("TP"))
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_comment && anno!="")
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_ihdb_hom && anno=="0")
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_ihdb_het && anno=="0")
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_clinvar && anno.contains("(confirmed)"))
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_genes)
			{
				bool hit = false;
				if (anno.contains(','))
				{
					 hit = imprinting_genes_.intersectsWith(GeneSet::createFromText(anno, ','));
				}
				else
				{
					hit = imprinting_genes_.contains(anno);
				}
				if (hit)
				{
					item->setBackgroundColor(Qt::yellow);
					item->setToolTip("Imprinting gene");
				}
			}

			setItem(r, 5+j, item);
		}

		//vertical headers - warning (red), notice (orange)
		QTableWidgetItem* item = createTableItem(QByteArray::number(i+1));
		item->setData(Qt::UserRole, i); //store variant index in user data (for selection methods)
		if (is_notice_line && !is_ok_line)
		{
			item->setForeground(QBrush(QColor(255, 135, 60)));
			QFont font;
			font.setWeight(QFont::Bold);
			item->setFont(font);
		}
		else if (is_warning_line && !is_ok_line)
		{
			item->setForeground(QBrush(Qt::red));
			QFont font;
			font.setWeight(QFont::Bold);
			item->setFont(font);
		}
		setVerticalHeaderItem(r, item);
	}
}

int VariantTable::columnIndex(const QString& column_name) const
{
	for(int i=0; i<columnCount(); ++i)
	{
		if (horizontalHeaderItem(i)->text()==column_name)
		{
			return i;
		}
	}

	return -1;
}

int VariantTable::selectedVariantIndex() const
{
	QList<int> indices = selectedVariantsIndices();

	if (indices.count()!=1) return -1;

	return indices[0];
}

QList<int> VariantTable::selectedVariantsIndices() const
{
	QList<int> output;

	QList<QTableWidgetSelectionRange> ranges = selectedRanges();
	foreach(const QTableWidgetSelectionRange& range, ranges)
	{
		for(int row=range.topRow(); row<=range.bottomRow(); ++row)
		{
			//get header (variant index is stored in user data)
			QTableWidgetItem* header = verticalHeaderItem(row);
			if (header==nullptr) THROW(ProgrammingException, "Variant table row header not set!");
			bool ok;
			output << header->data(Qt::UserRole).toInt(&ok);
			if (!ok) THROW(ProgrammingException, "Variant table row header user data '" + header->data(Qt::UserRole).toString() + "' not an integer!");
		}
	}

	std::sort(output.begin(), output.end());

	return output;
}

void VariantTable::clearContents()
{
	setRowCount(0);
	setColumnCount(0);
}

void VariantTable::resizeCells()
{
	GUIHelper::resizeTableCells(this, 200);

	//set mimumn width of chr, start, end
	if (columnWidth(0)<42)
	{
		setColumnWidth(0, 42);
	}
	if (columnWidth(1)<62)
	{
		setColumnWidth(1, 62);
	}
	if (columnWidth(2)<62)
	{
		setColumnWidth(2, 62);
	}

	//restrict REF/ALT column width
	for (int i=3; i<=4; ++i)
	{
		if (columnWidth(i)>80)
		{
			setColumnWidth(i, 80);
		}
	}
}

void VariantTable::resizeCellsCustom()
{
	GUIHelper::resizeTableCells(this, 50);

	//set mimumn width of chr, start, end
	if (columnWidth(0)<42)
	{
		setColumnWidth(0, 42);
	}
	if (columnWidth(1)<62)
	{
		setColumnWidth(1, 62);
	}
	if (columnWidth(2)<62)
	{
		setColumnWidth(2, 62);
	}

	//big
	int size_big = 400;
	int index = columnIndex("OMIM");
	if (index!=-1) setColumnWidth(index, size_big);

	//medium
	int size_med = 100;
	index = columnIndex("genotype");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("gene");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("variant_type");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("filter");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("ClinVar");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("HGMD");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("NGSD_hom");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("NGSD_het");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("NGSD_group");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("classification");
	if (index!=-1) setColumnWidth(index, size_med);
	index = columnIndex("gene_info");
	if (index!=-1) setColumnWidth(index, size_med);
}

void VariantTable::copyToClipboard(bool split_quality)
{
	// Data to be copied is not selected en bloc
	if (selectedRanges().count()!=1 && !split_quality)
	{
		//Create 2d list with empty QStrings (size equal to QTable in Main Window)
		QList< QList<QString> > data;
		for(int r=0;r<rowCount();++r)
		{
			QList<QString> line;
			for(int c=0;c<columnCount();++c)
			{
				line.append("");
			}
			data.append(line);
		}

		//Fill data with non-empty entries from QTable in Main Window
		QBitArray empty_columns;
		empty_columns.fill(true,columnCount());
		QList<QTableWidgetItem*> all_items = selectedItems();
		foreach(QTableWidgetItem* item,all_items)
		{
			if(!item->text().isEmpty())
			{
				data[item->row()][item->column()] = item->text();
				empty_columns[item->column()] = false;
			}
		}

		//Remove empty columns
		for(int c=columnCount()-1;c>=0;--c)
		{
			if(empty_columns[c])
			{
				for(int r=0;r<rowCount();++r)
				{
					data[r].removeAt(c);
				}
			}
		}

		//Remove empty rows
		for(int r=rowCount()-1;r>=0;--r)
		{
			bool row_is_empty = true;
			for(int c=0;c<data[r].count();++c)
			{
				if(!data[r][c].isEmpty())
				{
					row_is_empty = false;
					break;
				}
			}
			if(row_is_empty) data.removeAt(r);
		}

		QString text = "";
		for(int r=0;r<data.count();++r)
		{
			for(int c=0;c<data[r].count();++c)
			{
				text.append(data[r][c]);
				if(c<data[r].count()-1) text.append("\t");
			}
			text.append("\n");
		}
		QApplication::clipboard()->setText(text);

		return;
	}

	QTableWidgetSelectionRange range = selectedRanges()[0];

	//check quality column is present
	QStringList quality_keys;
	quality_keys << "QUAL" << "DP" << "AF" << "MQM" << "TRIO"; //if modified, also modify quality_values!!!
	int qual_index = -1;
	if (split_quality)
	{
		qual_index = columnIndex("quality");
		if (qual_index==-1)
		{
			QMessageBox::warning(this, "Copy to clipboard", "Column with index 6 has other name than quality. Aborting!");
			return;
		}
	}


	//copy header
	QString selected_text = "";
	if (range.rowCount()!=1)
	{
		selected_text += "#";
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");
			if (split_quality && col==qual_index)
			{
				selected_text.append(quality_keys.join('\t'));
			}
			else
			{
				selected_text.append(horizontalHeaderItem(col)->text());
			}
		}
	}

	//copy rows
	for (int row=range.topRow(); row<=range.bottomRow(); ++row)
	{
		if (selected_text!="") selected_text.append("\n");
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");

			QTableWidgetItem* current_item = item(row, col);
			if (current_item==nullptr) continue;

			if (split_quality && col==qual_index)
			{
				QStringList quality_values;
				for(int i=0; i<quality_keys.count(); ++i) quality_values.append("");
				QStringList entries = current_item->text().split(';');
				foreach(const QString& entry, entries)
				{
					QStringList key_value = entry.split('=');
					if (key_value.count()!=2)
					{
						QMessageBox::warning(this, "Copy to clipboard", "Cannot split quality entry '" + entry + "' into key and value. Aborting!");
						return;
					}
					int index = quality_keys.indexOf(key_value[0]);
					if (index==-1)
					{
						QMessageBox::warning(this, "Copy to clipboard", "Unknown quality entry '" + key_value[0] + "'. Aborting!");
						return;
					}

					quality_values[index] = key_value[1];
				}
				selected_text.append(quality_values.join('\t'));
			}
			else
			{
				selected_text.append(current_item->text().replace('\n',' ').replace('\r',' '));
			}
		}
	}

	QApplication::clipboard()->setText(selected_text);
}
