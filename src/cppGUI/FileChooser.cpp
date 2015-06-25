#include "FileChooser.h"
#include "Settings.h"
#include "GUIHelper.h"
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QMenu>
#include <QDesktopServices>
#include <QMimeData>
#include <QBoxLayout>

FileChooser::FileChooser(FileChooser::Type type, QWidget *parent)
	: QWidget(parent)
	, type_(type)
{
	setAcceptDrops(true);

	//create layout
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight);
	layout->setMargin(0);
	layout->setSpacing(3);
	setLayout(layout);

	//add widgets
	text_ = new QLineEdit();
	layout->addWidget(text_);
	button_ = new QPushButton("browse");
	layout->addWidget(button_);

	connect(button_, SIGNAL(clicked()), this, SLOT(browseFile()));
}

QString FileChooser::file()
{
	return text_->text();
}

void FileChooser::setFile(QString file)
{
	text_->setText(file);
}

void FileChooser::browseFile()
{
	QString file = "";

	if (type_==LOAD)
	{
		file = QFileDialog::getOpenFileName(this, "Select input file.", Settings::path("open_data_folder"), "*.*");
		if (file!="")
		{
			Settings::setPath("open_data_folder", file);
		}
	}
	else
	{
		file = QFileDialog::getSaveFileName(this, "Select output file.", Settings::path("store_data_folder"), "*.*");
		if (file!="")
		{
			Settings::setPath("store_data_folder", file);
		}
	}

	text_->setText(file);
	text_->setToolTip(file);
}

void FileChooser::dragEnterEvent(QDragEnterEvent* e)
{
	if (e->mimeData()->hasFormat("text/uri-list") && e->mimeData()->urls().count()==1)
	{
		e->acceptProposedAction();
	}
}

void FileChooser::dropEvent(QDropEvent* e)
{
	QString filename = e->mimeData()->urls().first().toLocalFile();

	if (filename.isEmpty())  return;

	text_->setText(filename);
}

void FileChooser::contextMenuEvent(QContextMenuEvent* e)
{
	//create menu
	QMenu* menu = new QMenu();
	menu->addAction("Copy");
	menu->addAction("Paste");
	menu->addAction("Delete");
	menu->addSeparator();
	if (text_->text()!="")
	{
		menu->addAction("Open");
	}

	//execute
	QAction* selected = menu->exec(e->globalPos());

	//evaluate
	if (selected!=0)
	{
		if(selected->text()=="Copy")
		{
			text_->selectAll();
			text_->copy();
			text_->deselect();
		}
		else if(selected->text()=="Paste")
		{
			QString previous = text_->text();

			//paste
			text_->selectAll();
			text_->paste();
			text_->deselect();

			//check file exists
			if (!QFile::exists(text_->text()))
			{
				GUIHelper::showMessage("Error", "File '" + text_->text() + "' does not exist!");
				text_->setText(previous);
			}
		}
		else if(selected->text()=="Delete")
		{
			text_->clear();
		}
		else if(selected->text()=="Open")
		{
			QDesktopServices::openUrl("file:///" + text_->text());
		}
	}
}
