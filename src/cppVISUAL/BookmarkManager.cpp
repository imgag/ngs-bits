#include "BookmarkManager.h"
#include "ui_BookmarkManager.h"
#include "Settings.h"
#include "SharedData.h"
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>

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
		auto* show_item = new QTableWidgetItem();
		show_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
		show_item->setCheckState(bookmark.value("show_in_chromosome_view").toBool(false) ? Qt::Checked : Qt::Unchecked);
		ui_->bookmarks_table->setItem(row, 2, show_item);
	}
	connect(ui_->bookmarks_table, &QTableWidget::itemChanged, this, &BookmarkManager::updateDeleteButton);
	connect(ui_->bookmarks_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item)
	{
		if (item->column() != 2) return;
		QJsonObject bookmark = QJsonDocument::fromJson(bookmarks_[item->row()].toUtf8()).object();
		bookmark["show_in_chromosome_view"] = item->checkState() == Qt::Checked;
		bookmarks_[item->row()] = QString::fromUtf8(QJsonDocument(bookmark).toJson(QJsonDocument::Compact));
		Settings::setStringList("bookmark_regions", bookmarks_);
		emit SharedData::instance()->bookmarksChanged();
	});
	connect(ui_->delete_btn, &QPushButton::clicked, this, &BookmarkManager::deleteCheckedBookmarks);
	connect(ui_->button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	ui_->bookmarks_table->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_->bookmarks_table, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos)
	{
		const int row = ui_->bookmarks_table->indexAt(pos).row();
		if (row < 0) return;
		ui_->bookmarks_table->selectRow(row);
		QMenu menu(this);
		menu.setObjectName("bookmark_context_menu");
		connect(menu.addAction("Rename"), &QAction::triggered, this, [this, row]() { renameBookmark(row); });
		menu.exec(ui_->bookmarks_table->viewport()->mapToGlobal(pos));
	});
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
	emit SharedData::instance()->bookmarksChanged();
	accept();
}

void BookmarkManager::renameBookmark(int row)
{
	QInputDialog dialog(this);
	dialog.setWindowTitle("Rename bookmark");
	dialog.setLabelText("Bookmark name:");
	dialog.setTextValue(ui_->bookmarks_table->item(row, 0)->text());
	dialog.findChild<QLineEdit*>()->selectAll();
	if (dialog.exec() != QDialog::Accepted || dialog.textValue().trimmed().isEmpty()) return;
	const QString name = dialog.textValue().trimmed();
	QJsonObject bookmark = QJsonDocument::fromJson(bookmarks_[row].toUtf8()).object();
	bookmark["name"] = name;
	bookmarks_[row] = QString::fromUtf8(QJsonDocument(bookmark).toJson(QJsonDocument::Compact));
	Settings::setStringList("bookmark_regions", bookmarks_);
	emit SharedData::instance()->bookmarksChanged();
	ui_->bookmarks_table->item(row, 0)->setText(name);
}
