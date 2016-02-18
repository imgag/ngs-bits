#include "SampleList.h"
#include <QKeyEvent>
#include <QDebug>
#include <QClipboard>
#include <QMessageBox>

SampleList::SampleList(QWidget *parent)
	: QTableWidget(parent)
	, ui()
{
	ui.setupUi(this);
	setColumnWidth(0, 150);
	setColumnWidth(1, 250);
	setColumnWidth(2, 250);
}

void SampleList::keyPressEvent(QKeyEvent* event)
{
	auto clip = QApplication::clipboard();

	//allow copy-paste without going to edit mode of cell
	QList<QTableWidgetItem*> items = selectedItems();
	if (items.count()==1)
	{
		if(event->matches(QKeySequence::Copy))
		{
			clip->setText(items[0]->text());
			event->accept();
			return;
		}
		if(event->matches(QKeySequence::Paste))
		{
			items[0]->setText(clip->text());
			event->accept();
			return;
		}
		if(event->matches(QKeySequence::Cut))
		{
			clip->setText(items[0]->text());
			items[0]->setText("");
			event->accept();
			return;
		}
		if(event->matches(QKeySequence::Delete))
		{
			items[0]->setText("");
			event->accept();
			return;
		}
	}

	//allow pasting from Excel
	if (items.count()==0 && event->matches(QKeySequence::Paste))
	{
		appendSamplesFromText(clip->text());
	}

	//default key-press event
	QTableWidget::keyPressEvent(event);
}

void SampleList::dropEvent(QDropEvent* event)
{
	//prevent drops outside of items
	QTableWidgetItem* item = itemAt(event->pos());
	if (item==nullptr)
	{
		event->setAccepted(false);
		return;
	}

	//default drop event
	QTableWidget::dropEvent(event);
}


void SampleList::appendSampleRO(QString name, QString a1, QString a2)
{
	appendSample(true, name, a1, a2);
}


void SampleList::appendSampleRW(QString name, QString a1, QString a2)
{
	appendSample(false, name, a1, a2);
}

void SampleList::appendSamplesFromText(QString text)
{
	QStringList rows = text.split("\n", QString::SkipEmptyParts);
	for (int i=0; i<rows.count(); ++i)
	{
		QStringList fields = rows[i].split("\t");
		if (fields.count()!=3)
		{
			QString tmp = rows[i];
			tmp.replace("\t", "[tab]");
			QMessageBox::critical(this, "Error appending sample(s) from text.", "Each row must consist of three tab-separated fields!\nThe row " + QString::number(i+1) +  " is invalid:\n" + tmp);
			return;
		}
		appendSampleRW(fields[0], fields[1], fields[2]);
	}
}

void SampleList::appendSample(bool read_only, QString name, QString a1, QString a2)
{
	//append a row
	int idx = rowCount();
	setRowCount(idx+1);

	//add items to the row
	setItem(idx, 0, createItem(read_only, name));
	setItem(idx, 1, createItem(read_only, a1));
	setItem(idx, 2, createItem(read_only, a2));
}

QTableWidgetItem* SampleList::createItem(bool read_only, QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem(text);
	if (read_only)
	{
		item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled);
	}
	else
	{
		item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled|Qt::ItemIsEnabled);
	}
	return item;
}
