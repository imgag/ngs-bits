#ifndef QCCOLLECTION_H
#define QCCOLLECTION_H

#include "cppNGS_global.h"
#include <QVariant>
#include <QVector>
#include <QtXml/QDomElement>

///QC value.
class CPPNGSSHARED_EXPORT QCValue
{
public:
	///Constructs an invalid QC value. Many containers need the elements to be default-constructible...
	QCValue();
	///Integer constructor.
	QCValue(const QString& name, int value, const QString& description="", const QString& accession="NONE");
	///Long long constructor.
	QCValue(const QString& name, long long value, const QString& description="", const QString& accession="NONE");
	///Float constructor.
	QCValue(const QString& name, double value, const QString& description="", const QString& accession="NONE");
	///String constructor.
	QCValue(const QString& name, const QString& value, const QString& description="", const QString& accession="NONE");

	///Base64-encoded PNG image creation function.
	static QCValue Image(const QString& name, const QString& filename, const QString& description="", const QString& accession="NONE");

	///Returns the name.
	const QString& name() const;
	///Returns the type.
	QVariant::Type type() const;
	///Returns the description.
	const QString& description() const;
	///Returns the accession, e.g. of an ontology.
	const QString& accession() const;

	///Returns the integer value - or throws a TypeConversionException if the QC value has a different type.
	int asInt() const;
	///Returns the long long value - or throws a TypeConversionException if the QC value has a different type.
	long long asLongLong() const;
	///Returns the integer value - or throws a TypeConversionException if the QC value has a different type.
	double asDouble() const;
	///Returns the integer value - or throws a TypeConversionException if the QC value has a different type.
	QString asString() const;
	///Returns the base64-encoded PNG image - or throws a TypeConversionException if the QC value has a different type.
	QByteArray asImage() const;
	///Returns the string representation of the value.
	QString toString(int double_precision = 2) const;

protected:
	QString name_;
	QVariant value_;
	QString description_;
	QString accession_;
};

///A collection of QC values.
class CPPNGSSHARED_EXPORT QCCollection
{
public:
	///Default constructor.
	QCCollection();

	///Inserts a value. Overwrites the value with the same name if it exists.
	void insert(const QCValue& value);
	///Inserts all terms/mages of a second QCCollection.
	void insert(const QCCollection& collection);
	///QC value accessor by index.
	const QCValue& operator[](int index) const;
	///QC value accessor by name (or accession). If no such value exists, @p ArgumentException in thrown.
	const QCValue& value(const QString& name, bool by_accession=false) const;
	///Returns the QC value count.
	int count() const;
	///Clears all terms.
	void clear();

	///Stores the collection to a qcML file. Double precitions for selected terms can be overwritten (default is 2).
	void storeToQCML(QString filename, const QStringList& source_files, QString parameters, QMap<QString, int> precision_overwrite = QMap<QString, int>(), QList<QCValue> metadata = QList<QCValue>());
	///Appends the terms to a string list, e.g. for text output. Skips PNG images. Double precisions for selected terms can be overwritten (default is 2).
	void appendToStringList(QStringList& list, QMap<QString, int> precision_overwrite = QMap<QString, int>());

	///reads a whole QCML file
	static QCCollection fromQCML(QString filename);



private:
	///Helper method to iterate through all elements recursively
	static void findElements(const QDomElement& elem, QList<QDomElement>& foundElements);
	///Recursive method to search a QDomElement (xml, qcml file) for elements with certain attributes, results are stored in foundElements
	static void findElementsWithAttributes(const QDomElement& elem, const QString& attr, QList<QDomElement>& foundElements);

protected:
	QList<QCValue> values_;
};

#endif // QCCOLLECTION_H
