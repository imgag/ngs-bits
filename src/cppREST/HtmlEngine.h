#ifndef HTMLENGINE_H
#define HTMLENGINE_H

#include "cppREST_global.h"
#include <QCoreApplication>
#include <QDateTime>
#include "Exceptions.h"
#include "Settings.h"

struct CPPRESTSHARED_EXPORT FolderItem
{
	QString name;
	bool is_folder;
	qint64 size;
	QDateTime modified;
};

typedef enum
{
	TO_PARENT_FOLDER,
	GENERIC_FILE,
	BINARY_FILE,
	CODE_FILE,
	PICTURE_FILE,
	TEXT_FILE,
	TABLE_FILE,
	FOLDER
} FolderItemIcon;

class CPPRESTSHARED_EXPORT HtmlEngine
{
public:
	static QString getPageHeader(QString page_title);
	static QString getPageFooter();
	static QString getApiHelpHeader(QString title);
	static QString getApiHelpEntry(QString url, QString method, QList<QString> param_names, QList<QString> param_desc, QString comment);
	static QString getErrorPageTemplate(QString title, QString message);

	static QString convertIconNameToString(FolderItemIcon in);
	static QString createFolderListingHeader(QString folder_name, QString parent_folder_link);
	static QString createFolderListingElements(QList<FolderItem> in, QString cur_folder_url);

	static QString getFolderIcons();
protected:
	HtmlEngine();	

private:
	static HtmlEngine& instance();

	const QList<QString> BINARY_EXT = {"bam", "exe"};
	const QList<QString> CODE_EXT = {"xml", "html", "yml"};
	const QList<QString> PICTURE_EXT = {"jpg", "jpeg", "png", "gif", "svg"};
	const QList<QString> TEXT_EXT = {"txt", "ini", "rtf", "doc", "docx"};
	const QList<QString> TABLE_EXT = {"csv", "xls", "xlsx"};


	static FolderItemIcon getIconType(FolderItem item);
	static QString createFolderItemLink(QString name, QString url, FolderItemIcon type);

};

#endif // HTMLENGINE_H
