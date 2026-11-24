#ifndef ADDBOOKMARKDIALOG_H
#define ADDBOOKMARKDIALOG_H

#include "cppVISUAL_global.h"
#include <QDialog>

namespace Ui { class AddBookmarkDialog; }

class CPPVISUALSHARED_EXPORT AddBookmarkDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AddBookmarkDialog(const QString& name, QWidget* parent = nullptr);
	~AddBookmarkDialog() override;

	QString name() const;
	bool showInChromosomeTrack() const;

private:
	Ui::AddBookmarkDialog* ui_;
};

#endif // ADDBOOKMARKDIALOG_H
