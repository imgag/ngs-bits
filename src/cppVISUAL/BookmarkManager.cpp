#include "BookmarkManager.h"
#include "ui_BookmarkManager.h"
#include "Settings.h"
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>

BookmarkManager::BookmarkManager(QWidget* parent)
	: QDialog(parent)
	, ui_(new Ui::BookmarkManager)
	, bookmarks_(Settings::stringList("bookmark_regions", true))
{
	ui_->setupUi(this);
	ui_->bookmarks_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui_->bookmarks_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui_->bookmarks_table->setRowCount(bookmarks_.count());
	for (int row = 0; row < bookmarks_.count(); ++row)
	{
		const QJsonObject bookmark = QJsonDocument::fromJson(bookmarks_[row].toUtf8()).object();
		auto* name_item = new QTableWidgetItem(bookmark.value("name").toString());
		name_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
		name_item->setCheckState(Qt::Unchecked);
		ui_->bookmarks_table->setItem(row, 0, name_item);
		const QString region = QString("%1:%2-%3").arg(bookmark.value("chr").toString()).arg(bookmark.value("start").toInt()).arg(bookmark.value("end").toInt());
		ui_->bookmarks_table->setItem(row, 1, new QTableWidgetItem(region));
	}
	connect(ui_->bookmarks_table, &QTableWidget::itemChanged, this, &BookmarkManager::updateDeleteButton);
	connect(ui_->delete_btn, &QPushButton::clicked, this, &BookmarkManager::deleteCheckedBookmarks);
	connect(ui_->button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	updateDeleteButton();
}

BookmarkManager::~BookmarkManager()
{
	delete ui_;
}

void BookmarkManager::updateDeleteButton()
{
	for (int row = 0; row < ui_->bookmarks_table->rowCount(); ++row)
	{
		if (ui_->bookmarks_table->item(row, 0)->checkState() == Qt::Checked)
		{
			ui_->delete_btn->setEnabled(true);
			return;
		}
	}
	ui_->delete_btn->setEnabled(false);
}

void BookmarkManager::deleteCheckedBookmarks()
{
	for (int row = ui_->bookmarks_table->rowCount() - 1; row >= 0; --row)
	{
		if (ui_->bookmarks_table->item(row, 0)->checkState() != Qt::Checked) continue;
		bookmarks_.removeAt(row);
		ui_->bookmarks_table->removeRow(row);
	}
	Settings::setStringList("bookmark_regions", bookmarks_);
	accept();
}
