#ifndef FILECHOOSER_H
#define FILECHOOSER_H

#include "cppGUI_global.h"
#include <QWidget>
#include <QContextMenuEvent>
#include <QLineEdit>
#include <QPushButton>

///A file selector for loading/storing with drag-and-drop support.
class CPPGUISHARED_EXPORT FileChooser
		: public QWidget
{
	Q_OBJECT

public:
	///The type.
	enum Type
		{
		LOAD,
		STORE
		};
	///Constructor.
	FileChooser(Type type, QWidget* parent = 0);
	///Returns the selected file.
	QString file();
	///Sets the selected file.
	void setFile(QString file);

protected:
	virtual void dragEnterEvent(QDragEnterEvent* e);
	virtual void dropEvent(QDropEvent* e);

protected slots:
	void browseFile();
	virtual void contextMenuEvent(QContextMenuEvent* e);

private:
	QLineEdit* text_;
	QPushButton* button_;
	Type type_;
};

#endif // FILECHOOSER_H
