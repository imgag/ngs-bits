#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include "cppVISUAL_global.h"
#include <QDialog>
#include <QStringList>

namespace Ui { class BookmarkManager; }

// Displays stored bookmarks and deletes checked entries in a batch.
class CPPVISUALSHARED_EXPORT BookmarkManager : public QDialog
{
	Q_OBJECT
public:
	explicit BookmarkManager(QWidget* parent = nullptr);
	~BookmarkManager() override;

private:
	Ui::BookmarkManager* ui_;
	QStringList bookmarks_;
	void updateDeleteButton();
	void deleteCheckedBookmarks();
};

#endif // BOOKMARKMANAGER_H
