#include "QCCollection.h"
#include "Exceptions.h"
#include "Helper.h"
#include "XmlHelper.h"
#include <QStringList>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileInfo>
#include <QList>
#include <QFile>
#include "OntologyTermCollection.h"

QCValue::QCValue()
	: name_("")
	, value_()
	, type_(QCValueType::STRING)
	, description_("")
{

}

QCValue::QCValue(const QString& name, int value, const QString& description, const QString& accession)
	: name_(name)
	, value_(value)
	, type_(QCValueType::INT)
	, description_(description)
	, accession_(accession)
{
}

QCValue::QCValue(const QString& name, long long value, const QString& description, const QString& accession)
	: name_(name)
	, value_(value)
	, type_(QCValueType::INT)
	, description_(description)
	, accession_(accession)
{
}

QCValue::QCValue(const QString& name, double value, const QString& description, const QString& accession)
	: name_(name)
	, value_(value)
	, type_(QCValueType::DOUBLE)
	, description_(description)
	, accession_(accession)
{
}

QCValue::QCValue(const QString& name, const QString& value, const QString& description, const QString& accession)
	: name_(name)
	, value_(value)
	, type_(QCValueType::STRING)
	, description_(description)
	, accession_(accession)
{
}

QCValue QCValue::ImageFromFile(const QString& name, const QString& filename, const QString& description, const QString& accession)
{
	//load file
	QByteArray data = "";
	if(!QFile::exists(filename)) THROW(FileAccessException, "QCValue::ImageFromFile: File '" + filename + "' does not exist!");
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly)) THROW(FileAccessException, "QCValue::ImageFromFile: File '" + filename + "' could not be opened!");
	data = file.readAll().toBase64();
	file.close();

	//create value
	QCValue value;
	value.name_ = name;
	value.value_ = data;
	value.type_ = QCValueType::IMAGE;
	value.description_ = description;
	value.accession_ = accession;

	return value;
}

QCValue QCValue::ImageFromText(const QString& name, const QByteArray& data_base64_encoded, const QString& description, const QString& accession)
{
	QCValue value;
	value.name_ = name;
	value.value_ = data_base64_encoded;
	value.type_ = QCValueType::IMAGE;
	value.description_ = description;
	value.accession_ = accession;

	return value;
}

long long QCValue::asInt() const
{
	if (type_!=QCValueType::INT) THROW(TypeConversionException, "QCValue '" + name_ + "' requested as integer, but has different type!");

	return value_.toLongLong();
}

double QCValue::asDouble() const
{
	if (type_!=QCValueType::DOUBLE) THROW(TypeConversionException, "QCValue '" + name_ + "' requested as double, but has different type!");

	return value_.toDouble();
}

QString QCValue::asString() const
{
	if (type_!=QCValueType::STRING) THROW(TypeConversionException, "QCValue '" + name_ + "' requested as string, but has different type!");

	return value_.toString();
}


QByteArray QCValue::asImage() const
{
	if (type_!=QCValueType::IMAGE) THROW(TypeConversionException, "QCValue '" + name_ + "' requested as image, but has different type!");

	return value_.toByteArray();
}

QString QCValue::toString(int double_precision) const
{
	if (type()==QCValueType::DOUBLE)
	{
		return QString::number(value_.toDouble(), 'f', double_precision);
	}

	return value_.toString();
}

QCCollection::QCCollection()
{
}

void QCCollection::insert(const QCValue& value)
{
	//check if it is already containd
	int index = -1;
	for (int i=0; i<count(); ++i)
	{
		if  (values_[i].name()==value.name())
		{
			index = i;
			break;
		}
	}

	//insert
	if (index==-1)
	{
		values_.append(value);
	}
	else
	{
		values_[index] = value;
	}
}

void QCCollection::insert(const QCCollection& collection)
{
	for (int i=0; i<collection.count(); ++i)
	{
		insert(collection[i]);
	}
}

bool QCCollection::contains(const QString& name, bool by_accession) const
{
	for (int i=0; i<count(); ++i)
	{
		if (!by_accession && values_[i].name()==name)
		{
			return true;
		}
		else if (by_accession && values_[i].accession()==name)
		{
			return true;
		}
	}
	return false;
}

const QCValue& QCCollection::value(const QString& name, bool by_accession) const
{
	for (int i=0; i<count(); ++i)
	{
		if (!by_accession && values_[i].name()==name)
		{
			return values_[i];
		}
		else if (by_accession && values_[i].accession()==name)
		{
			return values_[i];
		}
	}

	THROW(ArgumentException, "QC value with name/accession '" + name + "' not found in QC collection.")
}

void QCCollection::storeToQCML(QString filename, const QStringList& source_files, QString parameters, QMap<QString, int> precision_overwrite, QList<QCValue> metadata)
{
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename, true);
	QTextStream stream(file.data());

	//write header
	stream << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>" << endl;
	stream << "<?xml-stylesheet type=\"text/xml\" href=\"#stylesheet\"?>" << endl;
	stream << "<!DOCTYPE catelog [" << endl;
	stream << "  <!ATTLIST xsl:stylesheet" << endl;
	stream << "  id  ID  #REQUIRED>" << endl;
	stream << "  ]>" << endl;
	stream << "<qcML version=\"0.0.8\" xmlns=\"http://www.prime-xs.eu/ms/qcml\" ";
	stream << ">" << endl;
	stream << "  <runQuality ID=\"rq0001\">" << endl;

	//write meta data
	stream << "    <metaDataParameter ID=\"md0001\" name=\"creation software\" value=\"" << QCoreApplication::applicationName() <<" " << QCoreApplication::applicationVersion() << "\" cvRef=\"QC\" accession=\"QC:1000002\"/>" << endl;
	stream << "    <metaDataParameter ID=\"md0002\" name=\"creation software parameters\" value=\"" << parameters.toHtmlEscaped() << "\" cvRef=\"QC\" accession=\"QC:1000003\"/>" << endl;
	stream << "    <metaDataParameter ID=\"md0003\" name=\"creation date\" value=\"" << Helper::dateTime("") << "\" cvRef=\"QC\" accession=\"QC:1000004\"/>" << endl;
	int idx = 4;
	foreach(const QString& sf, source_files)
	{
		stream << "    <metaDataParameter ID=\"md" << QString::number(idx).rightJustified(4, '0') << "\" name=\"source file\" value=\"" << QFileInfo(sf).fileName() << "\" cvRef=\"QC\" accession=\"QC:1000005\"/>" << endl;
		++idx;
	}
	foreach(const QCValue& md, metadata)
	{
		if(md.accession()=="QC:1000006")	stream << "    <metaDataParameter ID=\"md" << QString::number(idx).rightJustified(4, '0') << "\" name=\"" << md.name() << "\" value=\"" << QFileInfo(md.asString()).fileName() << "\" uri=\"" << md.asString() << "\" cvRef=\"QC\" accession=\"" << md.accession() << "\" />" << endl;
		else	stream << "    <metaDataParameter ID=\"md" << QString::number(idx).rightJustified(4, '0') << "\" name=\"" << md.name() << "\" value=\"" << md.toString() << "\" cvRef=\"QC\" accession=\"" << md.accession() << "\"/>" << endl;
		++idx;
	}

	//write quality parameters
	for (int i=0; i<count(); ++i)
	{
		if (values_[i].type()==QCValueType::IMAGE) continue;

		QString name = values_[i].name();
		QString value = values_[i].toString();
		if (precision_overwrite.contains(name))
		{
			value = values_[i].toString(precision_overwrite[name]);
		}

		stream << "    <qualityParameter ID=\"qp" << QString::number(i+1).rightJustified(4, '0') << "\" name=\"" << name << "\" description=\"" << values_[i].description().toHtmlEscaped() << "\" value=\"" << value << "\" cvRef=\"QC\" accession=\"" << values_[i].accession() << "\"/>" << endl;
	}
	for (int i=0; i<count(); ++i)
	{
		if (values_[i].type()!=QCValueType::IMAGE) continue;
		stream << "    <attachment ID=\"qp" << QString::number(i+1).rightJustified(4, '0') << "\" name=\"" << values_[i].name() << "\" description=\"" << values_[i].description().toHtmlEscaped() << "\" cvRef=\"QC\" accession=\"" << values_[i].accession() << "\">" << endl;
		stream << "      <binary>" << values_[i].asImage() << "</binary>" << endl;
		stream << "    </attachment>" << endl;
	}
	stream << "  </runQuality>" << endl;

	//write CV list
	stream << "  <cvList>" << endl;
	stream << "    <cv uri=\"https://raw.githubusercontent.com/imgag/ngs-bits/master/src/cppNGS/Resources/qcML.obo\" ID=\"QC\" fullName=\"QC\" version=\"0.1\"/>" << endl;
	stream << "  </cvList>" << endl;

	//write stylesheet
	stream << "  <xsl:stylesheet id=\"stylesheet\" version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" xmlns:ns=\"http://www.prime-xs.eu/ms/qcml\" xmlns=\"\"";
	stream << ">" << endl;
	stream << "      <xsl:template match=\"/\">" << endl;
	stream << "          <html>" << endl;
	stream << "            <style type=\"text/css\">" << endl;
	stream << "            table {border: 1px solid #bbb; border-collapse: collapse; }" << endl;
	stream << "            td {border: 1px solid #bbb; padding: 1px 2px 1px 2px; vertical-align: top; }" << endl;
	stream << "            th {border: 1px solid #bbb; padding: 1px 2px 1px 2px; text-align: left; background-color: #eee; }" << endl;
	stream << "            </style>" << endl;
	stream << "              <body>" << endl;
	stream << "                  <h2>Meta data:</h2>" << endl;
	stream << "                  <table>" << endl;
	stream << "                    <tr>" << endl;
	stream << "                      <th>Accession</th><th>Name</th><th>Value</th>" << endl;
	stream << "                    </tr>" << endl;
	stream << "                    <xsl:for-each select=\"ns:qcML/ns:runQuality\">" << endl;
	stream << "                      <xsl:for-each select=\"ns:metaDataParameter\">" << endl;
	stream << "                        <tr>" << endl;
	stream << "                          <td><xsl:value-of select=\"@accession\"/></td>" << endl;
	stream << "                          <td><span title=\"{@description}\"><xsl:value-of select=\"@name\"/></span></td>" << endl;
	stream << "                          <td>" << endl;
	stream << "                            <xsl:choose>" << endl;
	stream << "                              <xsl:when test=\"@accession = 'QC:1000006'\"><a href=\"{@uri}\" title=\"{@uri}\" target=\"blank\"><xsl:value-of select=\"@value\"/></a></xsl:when>" << endl;
	stream << "                              <xsl:otherwise><xsl:value-of select=\"@value\"/></xsl:otherwise>" << endl;
	stream << "                            </xsl:choose>" << endl;
	stream << "                          </td>" << endl;
	stream << "                        </tr>" << endl;
	stream << "                      </xsl:for-each>" << endl;
	stream << "                    </xsl:for-each>" << endl;
	stream << "                  </table>" << endl;
	stream << "                  <h2>Quality parameters:</h2>" << endl;
	stream << "                  <table>" << endl;
	stream << "                    <tr>" << endl;
	stream << "                      <th>Accession</th><th>Name</th><th>Value</th>" << endl;
	stream << "                    </tr>" << endl;
	stream << "                    <xsl:for-each select=\"ns:qcML/ns:runQuality\">" << endl;
	stream << "                      <xsl:for-each select=\"ns:qualityParameter\">" << endl;
	stream << "                        <tr>" << endl;
	stream << "                          <td><xsl:value-of select=\"@accession\"/></td>" << endl;
	stream << "                          <td><span title=\"{@description}\"><xsl:value-of select=\"@name\"/></span></td>" << endl;
	stream << "                          <td><xsl:value-of select=\"@value\"/></td>" << endl;
	stream << "                        </tr>" << endl;
	stream << "                      </xsl:for-each>" << endl;
	stream << "                    </xsl:for-each>" << endl;
	stream << "                    <xsl:for-each select=\"ns:qcML/ns:runQuality\">" << endl;
	stream << "                      <xsl:for-each select=\"ns:attachment\">" << endl;
	stream << "                          <xsl:choose>" << endl;
	stream << "                              <xsl:when test=\"ns:binary\">" << endl;
	stream << "                                <tr>" << endl;
	stream << "                                  <td><xsl:value-of select=\"@accession\"/></td>" << endl;
	stream << "                                  <td><span title=\"{@description}\"><xsl:value-of select=\"@name\"/></span></td>" << endl;
	stream << "                                  <td>" << endl;
	stream << "                                    <img>" << endl;
	stream << "                                      <xsl:attribute name=\"src\">" << endl;
	stream << "                                        data:image/png;base64,<xsl:value-of select=\"ns:binary\"/>" << endl;
	stream << "                                      </xsl:attribute>" << endl;
	stream << "                                    </img>" << endl;
	stream << "                                  </td>" << endl;
	stream << "                                </tr>" << endl;
	stream << "                              </xsl:when>" << endl;
	stream << "                          </xsl:choose>" << endl;
	stream << "                      </xsl:for-each>" << endl;
	stream << "                    </xsl:for-each>" << endl;
	stream << "                  </table>" << endl;
	stream << "              </body>" << endl;
	stream << "          </html>" << endl;
	stream << "      </xsl:template>" << endl;
	stream << "  </xsl:stylesheet>" << endl;

	//finalize file
	stream << "</qcML>" << endl;
	file->close();

	//validate output
	if(filename!="")
	{
		//check schema
		QString xml_error = XmlHelper::isValidXml(filename, "://Resources/qcML_0.0.8.xsd");
		if (xml_error!="")
		{
			THROW(ProgrammingException, "QCCollection::storeToQCML produced an invalid XML file: " + xml_error);
		}

		//check used terms
		OntologyTermCollection terms("://Resources/qcML.obo", false);
		QList<QCValue> qc_values_used;
		qc_values_used.append(metadata);
		qc_values_used.append(values_);
		foreach(const QCValue& qc_value, qc_values_used)
		{
			QByteArray accession = qc_value.accession().toUtf8();
			if (!terms.containsByID(accession))
			{
				THROW(ProgrammingException, "QCCollection::storeToQCML produced an invalid XML file: QC term '" + accession + "/" + qc_value.name() + "' not found in the ontology!");
			}
			else if (terms.getByID(accession).isObsolete())
			{
				THROW(ProgrammingException, "QCCollection::storeToQCML produced an invalid XML file: QC term '" + accession + "/" + qc_value.name() + "' is marked as obsolete in the ontology!");
			}
		}
	}
}

void QCCollection::appendToStringList(QStringList& list, QMap<QString, int> precision_overwrite)
{
	for(int i=0; i<count(); ++i)
	{
		if (values_[i].type()==QCValueType::IMAGE) continue;

		QString name = values_[i].name();
		QString value = values_[i].toString();
		if (precision_overwrite.contains(name))
		{
			value = values_[i].toString(precision_overwrite[name]);
		}

		list << name + ": " + value;
	}
}

QCCollection QCCollection::fromQCML(QString filename, QString obo, QStringList& errors)
{
	QCCollection ouput;

	//check against schema
	QString xml_error = XmlHelper::isValidXml(filename, "://Resources/qcML_0.0.8.xsd");
	if (xml_error!="")
	{
		THROW(ProgrammingException, "QC file '" + filename + "' does not match schema: " + xml_error);
	}

	//load OBO
	OntologyTermCollection terms(obo, false);

	//open file
	QDomDocument doc;
	QString error_msg;
	int error_line, error_column;
	QFile f(filename);
	if(!doc.setContent(&f, &error_msg, &error_line, &error_column))
	{
		THROW(FileParseException, "QC file '" + filename + "' is invalid: " + error_msg + " line: " + QString::number(error_line) + " column: " +  QString::number(error_column));
	}
	
	//make list of all elements in doc
	QList<QDomElement> found_elements;
	QCCollection::findElements(doc.documentElement(),found_elements);
	foreach(const QDomElement& element, found_elements)
	{
		//only keep elements with qcML data
		if(element.nodeName()=="qualityParameter" || element.nodeName()=="attachment")
		{
			QString name = element.attribute("name");
			QString value = element.attribute("value");
			QByteArray accession = element.attribute("accession").toUtf8();
			QString description = element.attribute("description");

			//check accession exists
			if (!terms.containsByID(accession))
			{
				errors << "Skipped metric with unknown accession (accession=" + accession + "/" + name + ")";
				continue;
			}

			//convert value to correct type
			QCValue tmp;
			const OntologyTerm& term = terms.getByID(accession);
			if (term.type()=="int")
			{
				bool ok = false;
				long long num_value = value.toLongLong(&ok);
				if (!ok)
				{
					errors << "Skipped metric with invalid integer value '" + value + "' (accession=" + accession + "/" + name + ")";
					continue;
				}
				tmp = QCValue(name,num_value,description,accession);
			}
			else if (term.type()=="float")
			{
				bool ok = false;
				double num_value = value.toDouble(&ok);
				if (!ok)
				{
					errors << "Skipped metric with invalid float value '" + value + "' (accession=" + accession + "/" + name + ")";
					continue;
				}
				tmp = QCValue(name,num_value,description,accession);
			}
			else if (term.type()=="base64Binary") //image
			{
				QDomNode child = element.firstChild();
				if (child.isElement() && child.nodeName()=="binary")
				{
					tmp = QCValue::ImageFromText(name,child.toElement().text().toUtf8(),description,accession);
				}
			}
			else //everything else is handled as string
			{
				tmp = QCValue(name,value,description,accession);
			}
			ouput.insert(tmp);
		}
	}

	return ouput;
}

void QCCollection::findElements(const QDomElement &elem, QList<QDomElement>& foundElements)
{
	foundElements.append(elem);
	QDomElement child = elem.firstChildElement();

	while(!child.isNull())
	{
		QCCollection::findElements(child,foundElements);
		child = child.nextSiblingElement();
	}
}

void QCCollection::findElementsWithAttributes(const QDomElement &elem, const QString &attr, QList<QDomElement>& foundElements)
{
	if(elem.attributes().contains(attr)) foundElements.append(elem);
	QDomElement child = elem.firstChildElement();

	while(!child.isNull())
	{
		QCCollection::findElementsWithAttributes(child,attr,foundElements);
		child = child.nextSiblingElement();
	}
}
