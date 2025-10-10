#include "HtmlEngine.h"
#include "ToolBase.h"
#include <QTextStream>

HtmlEngine::HtmlEngine()
{
}

HtmlEngine& HtmlEngine::instance()
{
	static HtmlEngine html_engine;
	return html_engine;
}

QString HtmlEngine::getPageHeader(const QString& page_title)
{
	QString output;
	QTextStream stream(&output);

	stream << "<!doctype html>\n";
	stream << "<html lang=\"en\">\n";
	stream << "		<head>\n";
	stream << "			<meta charset=\"utf-8\">\n";
	stream << "			<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">\n";
	stream << "			<title>" << page_title << "</title>\n";
	stream << "			<style>\n";
	stream << "				html, body {\n";
	stream << "					padding: 10px;\n";
	stream << "					height: 100%;\n";
	stream << "				}\n";
	stream << "				hr {\n";
	stream << "					margin-top: 10px\n";
	stream << "				}\n";

	stream << "				* {\n";
	stream << "					box-sizing: border-box;\n";
	stream << "				}\n";
	stream << "				.column-10 {\n";
	stream << "					float: left;\n";
	stream << "					width: 10%;\n";
	stream << "					padding: 10px;\n";
	stream << "				}\n";
	stream << "				.column-25 {\n";
	stream << "					float: left;\n";
	stream << "					width: 25%;\n";
	stream << "					padding: 10px;\n";
	stream << "				}\n";
	stream << "				.column-33 {\n";
	stream << "					float: left;\n";
	stream << "					width: 33.33%;\n";
	stream << "					padding: 10px;\n";
	stream << "				}\n";
	stream << "				.row:after {\n";
	stream << "					content: \"\";\n";
	stream << "					display: table;\n";
	stream << "					clear: both;\n";
	stream << "				}\n";

	stream << "				.file-list {\n";
	stream << "					display: inline-block;\n";
	stream << "					vertical-align: middle;\n";
	stream << "					padding-bottom: 5px;\n";
	stream << "				}\n";
	stream << "				.file-list svg {\n";
	stream << "					margin-right: 5px;\n";
	stream << "				}\n";
	stream << "				.centered {\n";
	stream << "					text-align: center;\n";
	stream << "				}\n";
	stream << "				.main-content {\n";
	stream << "					min-height: 100%;\n";
	stream << "					min-height: 100vh;\n";
	stream << "					display: -webkit-box;\n";
	stream << "					display: -moz-box;\n";
	stream << "					display: -ms-flexbox;\n";
	stream << "					display: -webkit-flex;\n";
	stream << "					display: flex;\n";
	stream << "					-webkit-box-align: center;\n";
	stream << "					-webkit-align-items: center;\n";
	stream << "					-moz-box-align: center;\n";
	stream << "					-ms-flex-align: center;\n";
	stream << "					align-items: center;\n";
	stream << "					width: 100%;\n";
	stream << "					-webkit-box-pack: center;\n";
	stream << "					-moz-box-pack: center;\n";
	stream << "					-ms-flex-pack: center;\n";
	stream << "					-webkit-justify-content: center;\n";
	stream << "					justify-content: center;\n";
	stream << "				}\n";
	stream << "				.data-container {\n";
	stream << "					width: 640px;\n";
	stream << "				}\n";
	stream << "				pre {\n";
	stream << "					font-size: 14px;\n";
	stream << "				}\n";
	stream << "			</style>\n";
	stream << "		</head>\n";
	stream << "		<body>\n";
	return output;
}

QString HtmlEngine::getPageFooter()
{
	QString output;
	QTextStream stream(&output);
	stream << "			<hr>\n";
	stream << "			<p>" << ToolBase::applicationName() << " version " << ToolBase::version() << "</p>\n";
	stream << "		</body>\n";
	stream << "</html>\n";
	return output;
}

QString HtmlEngine::getApiHelpHeader(const QString& title)
{
	QString output;
	QTextStream stream(&output);

	stream << "			<h1>" << title << "</h1>\n";
	stream << "			<div class=\"row\">\n";
	stream << "				<div class=\"column-25\"><b>URL</b></div>\n";
	stream << "				<div class=\"column-25\"><b>Method</b></div>\n";
	stream << "				<div class=\"column-25\"><b>Parameters</b></div>\n";
	stream << "				<div class=\"column-25\"><b>Description</b></div>\n";
	stream << "			</div>\n";

	return output;
}

QString HtmlEngine::getApiHelpEntry(const QString& url, const QString& method, const QList<QString>& param_names, const QList<QString>& param_desc, const QString& comment)
{
	QString output;
	QTextStream stream(&output);

	stream << "			<div class=\"row\">\n";
	stream << "				<div class=\"column-25\">" << url << "</div>\n";
	stream << "				<div class=\"column-25\">" << method.toUpper() << "</div>\n";
	stream << "				<div class=\"column-25\">\n";

	for (int i = 0; i < param_names.count(); ++i)
	{
		stream << "				<b>" + param_names[i] + "</b> " + param_desc[i] + "<br />";
	}

	if (param_names.isEmpty())
	{
		stream << "				None";
	}
	stream << "				</div>\n";
	stream << "				<div class=\"column-25\">" << comment << "</div>\n";
	stream << "			</div>\n";

	return output;
}

QString HtmlEngine::getResponsePageTemplate(const QString& title, const QString& message)
{
	QString output;
	QTextStream stream(&output);

	stream << getPageHeader(title);
	stream << "			<div class=\"main-content\">\n";
	stream << "				<div class=\"data-container\">\n";
	stream << "					<h1 class=\"centered\">" << title << "</h1>\n";
	stream << "					<p>The following message indictates what actually has happened:</p>\n";
	stream << "					<pre>" << message << "</pre>\n";
	stream << "					<pre class=\"centered\">\n";
	stream << "O       o O       o O       o\n";
	stream << "| O   o | | O   o | | O   o |\n";
	stream << "| | O | | | | O | | | | O | |\n";
	stream << "| o   O | | o   O | | o   O |\n";
	stream << "o       O o       O o       O\n";
	stream << "					</pre>\n";
	stream << "				</div>\n";
	stream << "			</div>\n";
	stream << getPageFooter();

	return output;
}

QString HtmlEngine::convertIconNameToString(const FolderItemIcon& in)
{
	switch(in)
	{
		case TO_PARENT_FOLDER: return "up_dir";
		case GENERIC_FILE: return "generic_file";
		case BINARY_FILE: return "binary_file";
		case CODE_FILE: return "code_file";
		case PICTURE_FILE: return "picture_file";
		case TEXT_FILE: return "text_file";
		case TABLE_FILE: return "table_file";
		case FOLDER: return "folder";
	};
	return "";
}

QString HtmlEngine::createFolderListingHeader(const QString& folder_name, const QString& parent_folder_link)
{
	QString output;
	QTextStream stream(&output);


	stream << "			<h1>" << folder_name << "</h1><br />\n";
	stream << "			<div class=\"row\">\n";
	stream << "				<div class=\"column-33\">" << createFolderItemLink("to the parent folder", parent_folder_link, FolderItemIcon::TO_PARENT_FOLDER) << "</div>\n";
	stream << "				<div class=\"column-33\"><b>Size</b></div>\n";
	stream << "				<div class=\"column-33\"><b>Modified</b></div>\n";
	stream << "			</div>\n";

	return output;
}

QString HtmlEngine::createFolderListingElements(const QList<FolderItem>& in, const QString& cur_folder_url, const QString token)
{
	QString output;
	QTextStream stream(&output);

	for (int i = 0; i < in.count(); ++i)
	{
		long double size = in[i].size;
		QString size_units = "Bytes";
		if (in[i].size >= 1024)
		{
			size = size/1024.0;
			size_units = "KB";
		}

		if (in[i].size >= (1024*1024))
		{
			size = size/1024.0;
			size_units = "MB";
		}

		if (in[i].size >= (1024*1024*1024))
		{
			size = size/1024.0;
			size_units = "GB";
		}

		stream << "			<div class=\"row\">\n";
		stream << "				<div class=\"column-33\">" << HtmlEngine::createFolderItemLink(in[i].name, cur_folder_url + in[i].name + "?token=" + token, HtmlEngine::getIconType(in[i])) << "</div>\n";
		stream << "				<div class=\"column-33\">" << QString::number(size, 'g', 5) << " " << size_units <<"</div>\n";
		stream << "				<div class=\"column-33\">" << in[i].modified.toUTC().toString("hh:mm:ss dd.MM.yyyy") << "</div>\n";
		stream << "			</div>\n";
	}
	return output;
}

QString HtmlEngine::getFolderIcons()
{
	QString output;
	QTextStream stream(&output);

	stream << "			<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2em\" height=\"2em\" style=\"display: none;\" viewBox=\"0 0 16 16\">\n";
	stream << "				<symbol id=\"up_dir\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path fill-rule=\"evenodd\" d=\"M4.854 1.146a.5.5 0 0 0-.708 0l-4 4a.5.5 0 1 0 .708.708L4 2.707V12.5A2.5 2.5 0 0 0 6.5 15h8a.5.5 0 0 0 0-1h-8A1.5 1.5 0 0 1 5 12.5V2.707l3.146 3.147a.5.5 0 1 0 .708-.708l-4-4z\"/>\n";
	stream << "				</symbol>\n";
	stream << "				<symbol id=\"generic_file\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path d=\"M4 0h5.5v1H4a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V4.5h1V14a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2z\"/>\n";
	stream << "					<path d=\"M9.5 3V0L14 4.5h-3A1.5 1.5 0 0 1 9.5 3z\"/>\n";
	stream << "				</symbol>\n";
	stream << "				<symbol id=\"binary_file\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path fill-rule=\"evenodd\" d=\"M4 0h8a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2zm0 1a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H4z\"/>\n";
	stream << "					<path d=\"M5.526 13.09c.976 0 1.524-.79 1.524-2.205 0-1.412-.548-2.203-1.524-2.203-.978 0-1.526.79-1.526 2.203 0 1.415.548 2.206 1.526 2.206zm-.832-2.205c0-1.05.29-1.612.832-1.612.358 0 .607.247.733.721L4.7 11.137a6.749 6.749 0 0 1-.006-.252zm.832 1.614c-.36 0-.606-.246-.732-.718l1.556-1.145c.003.079.005.164.005.249 0 1.052-.29 1.614-.829 1.614zm5.329.501v-.595H9.73V8.772h-.69l-1.19.786v.688L8.986 9.5h.05v2.906h-1.18V13h3z\"/>\n";
	stream << "				</symbol>\n";
	stream << "				<symbol id=\"code_file\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path d=\"M4 0h5.5v1H4a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V4.5h1V14a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2z\"/>\n";
	stream << "					<path d=\"M9.5 3V0L14 4.5h-3A1.5 1.5 0 0 1 9.5 3z\"/>\n";
	stream << "					<path fill-rule=\"evenodd\" d=\"M8.646 6.646a.5.5 0 0 1 .708 0l2 2a.5.5 0 0 1 0 .708l-2 2a.5.5 0 0 1-.708-.708L10.293 9 8.646 7.354a.5.5 0 0 1 0-.708zm-1.292 0a.5.5 0 0 0-.708 0l-2 2a.5.5 0 0 0 0 .708l2 2a.5.5 0 0 0 .708-.708L5.707 9l1.647-1.646a.5.5 0 0 0 0-.708z\"/>\n";
	stream << "				</symbol>\n";
	stream << "				<symbol id=\"picture_file\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path fill-rule=\"evenodd\" d=\"M12 16a2 2 0 0 0 2-2V4.5L9.5 0H4a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h8zM3 2a1 1 0 0 1 1-1h5.5v2A1.5 1.5 0 0 0 11 4.5h2V10l-2.083-2.083a.5.5 0 0 0-.76.063L8 11 5.835 9.7a.5.5 0 0 0-.611.076L3 12V2z\"/>\n";
	stream << "					<path fill-rule=\"evenodd\" d=\"M6.502 7a1.5 1.5 0 1 0 0-3 1.5 1.5 0 0 0 0 3z\"/>\n";
	stream << "				</symbol>\n";
	stream << "				<symbol id=\"text_file\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path d=\"M4 0h5.5v1H4a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V4.5h1V14a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2z\"/>\n";
	stream << "					<path d=\"M9.5 3V0L14 4.5h-3A1.5 1.5 0 0 1 9.5 3z\"/>\n>";
	stream << "					<path fill-rule=\"evenodd\" d=\"M5 11.5a.5.5 0 0 1 .5-.5h2a.5.5 0 0 1 0 1h-2a.5.5 0 0 1-.5-.5zm0-2a.5.5 0 0 1 .5-.5h5a.5.5 0 0 1 0 1h-5a.5.5 0 0 1-.5-.5zm0-2a.5.5 0 0 1 .5-.5h5a.5.5 0 0 1 0 1h-5a.5.5 0 0 1-.5-.5z\"/>\n";
	stream << "				</symbol>\n";
	stream << "				<symbol id=\"table_file\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path fill-rule=\"evenodd\" d=\"M5 10H3V9h10v1h-3v2h3v1h-3v2H9v-2H6v2H5v-2H3v-1h2v-2zm1 0v2h3v-2H6z\"/>\n";
	stream << "					<path d=\"M4 0h5.5v1H4a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h8a1 1 0 0 0 1-1V4.5h1V14a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V2a2 2 0 0 1 2-2z\"/>\n";
	stream << "					<path d=\"M9.5 3V0L14 4.5h-3A1.5 1.5 0 0 1 9.5 3z\"/>\n";
	stream << "				</symbol>\n";
	stream << "				<symbol id=\"folder\" viewBox=\"0 0 16 16\">\n";
	stream << "					<path d=\"M9.828 4a3 3 0 0 1-2.12-.879l-.83-.828A1 1 0 0 0 6.173 2H2.5a1 1 0 0 0-1 .981L1.546 4h-1L.5 3a2 2 0 0 1 2-2h3.672a2 2 0 0 1 1.414.586l.828.828A2 2 0 0 0 9.828 3v1z\"/>\n";
	stream << "					<path fill-rule=\"evenodd\" d=\"M13.81 4H2.19a1 1 0 0 0-.996 1.09l.637 7a1 1 0 0 0 .995.91h10.348a1 1 0 0 0 .995-.91l.637-7A1 1 0 0 0 13.81 4zM2.19 3A2 2 0 0 0 .198 5.181l.637 7A2 2 0 0 0 2.826 14h10.348a2 2 0 0 0 1.991-1.819l.637-7A2 2 0 0 0 13.81 3H2.19z\"/>\n";
	stream << "				</symbol>\n";
	stream << "			</svg>\n";

	return output;
}


FolderItemIcon HtmlEngine::getIconType(const FolderItem& item)
{
	if (item.is_folder) return FolderItemIcon::FOLDER;

	QString found_extention = "";
	QList<QString> name_items = item.name.split(".");
	if (name_items.count()>0) found_extention = name_items[name_items.count()-1];

	if (HtmlEngine::instance().BINARY_EXT.indexOf(found_extention)>-1) return FolderItemIcon::BINARY_FILE;
	if (HtmlEngine::instance().CODE_EXT.indexOf(found_extention)>-1) return FolderItemIcon::CODE_FILE;
	if (HtmlEngine::instance().PICTURE_EXT.indexOf(found_extention)>-1) return FolderItemIcon::PICTURE_FILE;
	if (HtmlEngine::instance().TEXT_EXT.indexOf(found_extention)>-1) return FolderItemIcon::TEXT_FILE;
	if (HtmlEngine::instance().TABLE_EXT.indexOf(found_extention)>-1) return FolderItemIcon::TABLE_FILE;

	return FolderItemIcon::GENERIC_FILE;
}

QString HtmlEngine::createFolderItemLink(const QString& name, const QString& url, const FolderItemIcon& type)
{
	return "<a class=\"file-list\" href=\"" +url + "\"><svg class=\"file-list\" width=\"2em\" height=\"2em\"><use xlink:href=\"#" + convertIconNameToString(type) + "\" /></svg> <span>" + name + "</span></a>";
}
