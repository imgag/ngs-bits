#include "SessionManager.h"
#include "SharedData.h"

#include <QXmlStreamWriter>

const QString SessionManager::VERSION = "1.0.0";

void SessionManager::saveSession(QString file_path)
{
	QFile file(file_path);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to open file for writing:" << file.errorString();
		return;
	}

	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	writer.writeStartDocument();

	writer.writeStartElement("GSViewerSession");

	writer.writeAttribute("version", VERSION);

	writer.writeStartElement("General");
	SharedData::writeToXml(writer);
	writer.writeEndElement(); // General


}

QString SessionManager::version()
{
	return VERSION;
}
