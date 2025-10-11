#ifndef HTMLENGINE_H
#define HTMLENGINE_H

#include "cppREST_global.h"
#include <QDateTime>

struct CPPRESTSHARED_EXPORT FolderItem
{
	QString name;
	bool is_folder;
	qint64 size;
	QDateTime modified;
};

enum FolderItemIcon
{
	TO_PARENT_FOLDER,
	GENERIC_FILE,
	BINARY_FILE,
	CODE_FILE,
	PICTURE_FILE,
	TEXT_FILE,
	TABLE_FILE,
	FOLDER
};

class CPPRESTSHARED_EXPORT HtmlEngine
{
public:
	static QString getPageHeader(const QString& page_title);
	static QString getPageFooter();
	static QString getApiHelpHeader(const QString& title);
	static QString getApiHelpEntry(const QString& url, const QString& method, const QList<QString>& param_names, const QList<QString>& param_desc, const QString& comment);
	static QString getResponsePageTemplate(const QString& title, const QString& message);

	static QString convertIconNameToString(const FolderItemIcon& in);
	static QString createFolderListingHeader(const QString& folder_name, const QString& parent_folder_link);
	static QString createFolderListingElements(const QList<FolderItem>& in, const QString& cur_folder_url, const QString token);

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


	static FolderItemIcon getIconType(const FolderItem& item);
	static QString createFolderItemLink(const QString& name, const QString& url, const FolderItemIcon& type);

};

#endif // HTMLENGINE_H
