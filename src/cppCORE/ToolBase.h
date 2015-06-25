#ifndef TOOLBASE_H
#define TOOLBASE_H

#include "cppCORE_global.h"
#include <QCoreApplication>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <limits>

///Base class for command-line tools that handles parameter parsing and uncaught exceptions
class CPPCORESHARED_EXPORT ToolBase
		: public QCoreApplication
{
	Q_OBJECT

public:
	///Constructor
	ToolBase(int& argc, char *argv[]);
	///Initializes the tool. If @p version is unset, the cppNGS library version is taken
	void setDescription(QString description);
	///Setup method. In this method the init() method is called and parameters are added.
	virtual void setup() = 0;
	///Main method that contains the actual tool code.
	virtual void main() = 0;
	///Starts the event loop and excutes the setup() and main() methods.
	int execute();
	///Returns the application version
	static QString version();

	/**
	  @name Parameter handling methods
	 */
	//@{
	void addFlag(QString name, QString desc);
	void addInt(QString name, QString desc, bool optional, int default_value=0);
	void addFloat(QString name, QString desc, bool optional, double default_value=0.0);
	void addEnum(QString name, QString desc, bool optional, QStringList values, QString default_value="");
	void addString(QString name, QString desc, bool optional, QString default_value="");
	void addInfile(QString name, QString desc, bool optional, bool check_readable=true);
	void addOutfile(QString name, QString desc, bool optional, bool check_writable=true);
	void addInfileList(QString name, QString desc, bool optional, bool check_readable=true);

	bool getFlag(QString name) const;
	int getInt(QString name) const;
	double getFloat(QString name) const;
	QString getString(QString name) const;
	QString getEnum(QString name) const;
	QString getInfile(QString name) const;
	QString getOutfile(QString name) const;
	QStringList getInfileList(QString name) const;

	void printVersion() const;
	void printHelp() const;
	void storeTDXml() const;
	//@}

private slots:
	///Main method that parses command line parameters and excutes the run() method using the Qt event loop
	void executeInternal();

private:
	///Parameter type enumeration
	enum ParameterType
		{
		NONE,
		FLAG,
		INT,
		FLOAT,
		ENUM,
		STRING,
		INFILE,
		OUTFILE,
		INFILELIST
		};

	///Parameter data (declaration and parsed value)
	struct ParameterData
	{
		///Default constructor
		ParameterData();
		///Convenience constructor
        ParameterData(QString n, ParameterType t, QString d, bool o, QVariant v);

        QString name;
		ParameterType type;
		QString desc;
		bool optional;
		QVariant default_value;
		QMap<QString, QVariant> options;

		///Value after command line parsing
		QVariant value;
	};

	QString description_;
    QVector<ParameterData> parameters_;

    int parameterIndex(QString name) const;
    void addParameter(const ParameterData& data);
    int checkParameterExists(QString name, ParameterType type) const;

	///Parses the command line arguments and returns if the execution of the tool can go on (after special parameters like --version the execution is stopped).
	bool parseCommandLine();
	QString typeToString(ParameterType type) const;
	QString typeToArgString(ParameterType type) const;

	/// Default constructor "declared away"
	ToolBase();
	/// Copy constructor "declared away"
	ToolBase(const ToolBase&);

	///Reimplemented nofify method to handle exceptions
	bool notify(QObject* receiver, QEvent* event);

};

#endif //TOOLBASE_H




