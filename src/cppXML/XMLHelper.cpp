#include "XmlHelper.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QXmlSchemaValidator>
#include <QXmlSchema>
#include <QUrl>
#include <QTemporaryFile>

QString XmlHelper::isValidXml(QString xml_name, QString schema_name)
{
	//create schema url (both for native files and files from resources)
	QUrl schema_url;
	QScopedPointer<QTemporaryFile> tmp_file(QTemporaryFile::createNativeFile(schema_name));
	if (tmp_file!=0)
	{
		schema_url = QUrl::fromLocalFile(tmp_file->fileName());
	}
	else
	{
		schema_url = QUrl::fromLocalFile(schema_name);
	}

	//load schema
	QXmlSchema schema;
	if (!schema.load(schema_url))
	{
		THROW(FileParseException, "XML schema '" + schema_url.toString() + "'  is not valid/present.");
	}

	//validate file
	QXmlSchemaValidator validator(schema);
	XmlValidationMessageHandler handler;
	validator.setMessageHandler(&handler);
	QScopedPointer<QFile> xml_file(Helper::openFileForReading(xml_name));
	if (validator.validate(xml_file.data(), schema_url))
	{
		return "";
	}

	return handler.messages();
}

QString XmlHelper::XmlValidationMessageHandler::messages()
{
	return messages_;
}

void XmlHelper::XmlValidationMessageHandler::handleMessage(QtMsgType /*type*/, const QString& description, const QUrl& /*identifier*/, const QSourceLocation& /*sourceLocation*/)
{
	messages_ = messages_ + description + " ";
}
