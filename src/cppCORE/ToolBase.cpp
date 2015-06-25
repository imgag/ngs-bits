#include "ToolBase.h"
#include <QTextStream>
#include <QFileInfo>
#include <QTimer>
#include <QDir>
#include "Exceptions.h"
#include "Helper.h"

#define XSTR(x) #x
#define STR(x) XSTR(x)

ToolBase::ToolBase(int& argc, char *argv[])
	: QCoreApplication(argc, argv)
{
	QCoreApplication::setApplicationVersion(version());
}

QString ToolBase::version()
{
	return QString(STR(CPPCORE_VERSION));
}

void ToolBase::setDescription(QString description)
{
	description_ = description;
}

void ToolBase::addFlag(QString name, QString desc)
{
    ParameterData data(name, FLAG, desc, true, false);
    addParameter(data);
}

void ToolBase::addInt(QString name, QString desc, bool optional, int default_value)
{
    ParameterData data(name, INT, desc, optional, default_value);
    addParameter(data);
}

void ToolBase::addFloat(QString name, QString desc, bool optional, double default_value)
{
    ParameterData data(name, FLOAT, desc, optional, default_value);
    addParameter(data);
}

void ToolBase::addEnum(QString name, QString desc, bool optional, QStringList values, QString default_value)
{
	if (optional && !values.contains(default_value))
	{
		THROW(ProgrammingException, "Optional enum parameter '" + name + "' has invalid default value '" + default_value + "'. Valid are: '" + values.join(",") + "'!");
	}

    ParameterData data(name, ENUM, desc, optional, default_value);
	data.options.insert("values", values);
    addParameter(data);
}

void ToolBase::addString(QString name, QString desc, bool optional, QString default_value)
{
    ParameterData data(name, STRING, desc, optional, default_value);
    addParameter(data);
}

void ToolBase::addInfile(QString name, QString desc, bool optional, bool check_readable)
{
    ParameterData data(name, INFILE, desc, optional, "");
	data.options.insert("check_readable", check_readable);
    addParameter(data);
}

void ToolBase::addOutfile(QString name, QString desc, bool optional, bool check_writable)
{
    ParameterData data(name, OUTFILE, desc, optional, "");
	data.options.insert("check_writable", check_writable);
    addParameter(data);
}

void ToolBase::addInfileList(QString name, QString desc, bool optional, bool check_readable)
{
    ParameterData data(name, INFILELIST, desc, optional, "");
	data.options.insert("check_readable", check_readable);
    addParameter(data);
}

bool ToolBase::getFlag(QString name) const
{
    int index = checkParameterExists(name, FLAG);
    return parameters_[index].value.toBool();
}

int ToolBase::getInt(QString name) const
{
    int index = checkParameterExists(name, INT);
    return parameters_[index].value.toInt();
}

double ToolBase::getFloat(QString name) const
{
    int index = checkParameterExists(name, FLOAT);
    return parameters_[index].value.toDouble();
}

QString ToolBase::getString(QString name) const
{
    int index = checkParameterExists(name, STRING);
    return parameters_[index].value.toString();
}

QString ToolBase::getEnum(QString name) const
{
    int index = checkParameterExists(name, ENUM);
    return parameters_[index].value.toString();
}

QString ToolBase::getInfile(QString name) const
{
    int index = checkParameterExists(name, INFILE);
    return parameters_[index].value.toString();
}

QString ToolBase::getOutfile(QString name) const
{
    int index = checkParameterExists(name, OUTFILE);
    return parameters_[index].value.toString();
}

QStringList ToolBase::getInfileList(QString name) const
{
    int index = checkParameterExists(name, INFILELIST);
    return parameters_[index].value.toStringList();
}

bool ToolBase::parseCommandLine()
{
	QStringList args = QCoreApplication::arguments();

    //no arguments => show help if a mandatory parameter is present
    if (args.count()==1)
    {
        foreach(const ParameterData& data, parameters_)
        {
            if (!data.optional)
            {
                printHelp();
                return false;
            }
        }
    }

	//special parameters
	if(args.contains("--help") || args.contains("--h") || args.contains("-help"))
	{
		printHelp();
		return false;
	}
	if(args.contains("--version"))
	{
		printVersion();
		return false;
	}
	if(args.contains("--tdx"))
	{
		storeTDXml();
		return false;
	}

	//parse command line
	for (int i=1; i<args.count(); ++i)
	{
		QString par = args[i];

		//error: trailing argument
		if (par[0] != '-')
		{
			THROW(CommandLineParsingException, "Trailing parameter '" + par + "' given.");
		}
		par = par.mid(1);

		//error: parameter unknown
        int index = parameterIndex(par);
        if (index==-1)
		{
			THROW(CommandLineParsingException, "Unknown parameter '" + par + "' given.");
		}

        ParameterData& data = parameters_[index];

		//error: parameter without argument
		if (data.type!=FLAG && (i+1>=args.count() || (args[i+1]!="-" && args[i+1][0]=='-')))
		{
			THROW(CommandLineParsingException, "Parameter '" + par + "' given without argument.");
		}

		//error: parameter given more than once
		if (data.value.isValid())
		{
			THROW(CommandLineParsingException, "Parameter '" + par + "' given more than once.");
		}

		//FLAG
		if (data.type==FLAG)
		{
			data.value = true;
		}
		//INT
		else if (data.type==INT)
		{
			++i;
			bool ok = false;
			data.value = args[i].toInt(&ok);
			if (!ok)
			{
				THROW(CommandLineParsingException, "Value '" + args[i] + "' given for parameter '" + par + "' cannot be converted to integer.");
			}
		}
		//FLOAT
		else if (data.type==FLOAT)
		{
			++i;
			bool ok = false;
			data.value = args[i].toDouble(&ok);
			if (!ok)
			{
				THROW(CommandLineParsingException, "Value '" + args[i] + "' given for parameter '" + par + "' cannot be converted to float.");
			}
		}
		//ENUM
		else if (data.type==ENUM)
		{
			++i;
			QStringList values = data.options["values"].toStringList();
			if (!values.contains(args[i]))
			{
				THROW(CommandLineParsingException, "Value '" + args[i] +"' given for enum parameter '" + par + "' is not valid. Valid are: '" + values.join(",") + "'.");
			}
			data.value = args[i];
		}
		//STRING
		else if (data.type==STRING)
		{
			++i;
			data.value = args[i];
		}
		//INFILE
		else if (data.type==INFILE)
		{
			++i;

			if (args[i]!="" && data.options["check_readable"].toBool())
			{
				if (!QFileInfo(args[i]).isReadable())
				{
					THROW(CommandLineParsingException, "Input file '" + args[i] +"' given for parameter '" + par + "' is not readable.");
				}
			}

			data.value = args[i];
		}
		//OUTFILE
		else if (data.type==OUTFILE)
		{
			++i;

			if (args[i]!="" && data.options["check_writable"].toBool())
			{
				if (QFile::exists(args[i]))
				{
					QFileInfo file_info(args[i]);
					if (!file_info.isFile() || !file_info.isWritable())
					{
						THROW(CommandLineParsingException, "Output file '" + args[i] +"' given for parameter '" + par + "' is not writable.");
					}
				}
				else
				{
					QString dir = QFileInfo(args[i]).absolutePath();
					QFileInfo dir_info(dir);
					if (!dir_info.isDir()  || !dir_info.isWritable())
					{
						THROW(CommandLineParsingException, "Output file directory '" + dir + "' given for parameter '" + par + "' is not writable.");
					}
				}
			}

			data.value = args[i];
		}
		//INFILELIST
		else if (data.type==INFILELIST)
		{
			QStringList files;
			int j=i+1;
			while(j<args.count() && args[j][0]!='-')
			{
				if (data.options["check_readable"].toBool())
				{
					if (!QFileInfo(args[j]).isReadable())
					{
						THROW(CommandLineParsingException, "Input file '" + args[j] +"' given for parameter '" + par + "' is not readable.");
					}
				}

				files.append(args[j]);
				++j;
			}

			i=j-1;
			data.value = files;
		}
		//UNKNOWN
		else
		{
			THROW(ProgrammingException, "Unknown ToolBase parameter type!");
		}
	}

    //check for missing mandatory parameters
    for (int i=0; i<parameters_.count(); ++i)
    {
        ParameterData& data = parameters_[i];
        if (data.optional && !data.value.isValid())
		{
            data.value = data.default_value;
		}
        else if(!data.value.isValid())
		{
            THROW(CommandLineParsingException, "Mandatory parameter '" + data.name + "' not given.");
		}
    }

	return true;
}

void ToolBase::printVersion() const
{
	QTextStream stream(stdout);
	stream << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;
	stream << endl;
}

void ToolBase::printHelp() const
{
	QTextStream stream(stdout);

	// find out longest parameter name and matching offset
	int max_name = 0;
    foreach(const ParameterData& data, parameters_)
    {
        max_name = std::max(max_name, data.name.length() + typeToArgString(data.type).length());
	}
	int offset = std::max(13, max_name + 5);

	//print general info
	stream << QCoreApplication::applicationName() + " (" + QCoreApplication::applicationVersion() + ")" << endl;
	stream << "" << endl;
	stream << description_ << endl;

	// print mandatory parameters
    bool first_mandatory = true;
    foreach(const ParameterData& data, parameters_)
    {
		if (data.optional) continue;

		//header
		if (first_mandatory)
		{
			stream << "" << endl;
			stream << "Mandatory parameters:" << endl;
			first_mandatory = false;
		}

		//standard output
        QString line_start = "  -" + data.name + " " + typeToArgString(data.type);
		stream << line_start.leftJustified(offset, ' ') << data.desc << endl;

		//special handling of ENUM
		if (data.type==ENUM)
		{
			stream << QString(offset, ' ') << "Valid: '" << data.options["values"].toStringList().join(',') + "'" << endl;
		}
	}

	// print optional parameters
	bool first_optional = true;
    foreach(const ParameterData& data, parameters_)
    {
		if (!data.optional) continue;

		//header
		if (first_optional)
		{
			stream << "" << endl;
			stream << "Optional parameters:" << endl;
			first_optional = false;
		}

		//standard output
        QString line_start = "  -" + data.name + " " + typeToArgString(data.type);
		stream << line_start.leftJustified(offset, ' ') << data.desc << endl;
		stream << QString(offset, ' ') << "Default value: '" << data.default_value.toString() << "'" << endl;

		//special handling of ENUM
		if (data.type==ENUM)
		{
			stream << QString(offset, ' ') << "Valid: '" << data.options["values"].toStringList().join(',') + "'" << endl;
		}
	}

	// print special parameters
	stream << "" << endl;
	stream << "Special parameters:" << endl;
	stream << QString("  --help").leftJustified(offset, ' ') << "Shows this help and exits." << endl;
	stream << QString("  --version").leftJustified(offset, ' ') << "Prints version and exits." << endl;
	stream << QString("  --tdx").leftJustified(offset, ' ') << "Writes a Tool Defition Xml file. The file name is the application name appended with '.tdx'." << endl;
	stream << "" << endl;
}

void ToolBase::storeTDXml() const
{
	QString filename = QCoreApplication::applicationFilePath() + ".tdx";

	//write to stream
	QScopedPointer<QFile> out_file(Helper::openFileForWriting(filename));
	QTextStream stream(out_file.data());
	stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;
	stream << "<TDX version=\"1\">" << endl;
	stream << "  <Tool name=\"" << QCoreApplication::applicationName() << "\" version=\"" << QCoreApplication::applicationVersion() << "\">" << endl;
	stream << "    <Description>" << description_ << "</Description>" << endl;

    foreach(const ParameterData& data, parameters_)
    {
		QString tag_name = typeToString(data.type).toLower();
		tag_name[0] = tag_name[0].toUpper();
		tag_name.replace("list", "List");

        stream << "    <" << tag_name << " name=\"" << data.name << "\">" << endl;
		stream << "      <Description>" << data.desc << "</Description>" << endl;
		switch (data.type)
		{
			case STRING:
			case INT:
			case FLOAT:
				if (data.optional)
				{
					stream << "      <Optional defaultValue=\"" << data.default_value.toString() << "\" />" << endl;
				}
				break;
			case INFILE:
			case INFILELIST:
			case OUTFILE:
				if (data.optional)
				{
					stream << "      <Optional />" << endl;
				}
				break;
			case ENUM:
				stream << "      <Optional defaultValue=\"" << data.default_value.toString() << "\" />" << endl;
				foreach(const QString& value, data.options["values"].toStringList())
				{
					stream << "      <Value>" << value << "</Value>" << endl;
				}
				break;
			case FLAG:
			case NONE:
				break;
		}
		stream << "    </" << tag_name << ">" << endl;
	}
	stream << "  </Tool>" << endl;
	stream << "</TDX>" << endl;
}

int ToolBase::execute()
{
	QTimer::singleShot(0, this, SLOT(executeInternal()));
	return exec();
}

void ToolBase::executeInternal()
{
	setup();

	//execute main method only if no special parameters were set
	if (parseCommandLine())
	{
		main();
	}

    exit(0);
}

int ToolBase::parameterIndex(QString name) const
{
    for (int i=0; i<parameters_.count(); ++i)
    {
        if (parameters_[i].name==name) return i;
    }
    return -1;
}

void ToolBase::addParameter(const ToolBase::ParameterData& data)
{
    if (parameterIndex(data.name)!=-1)
	{
        THROW(ProgrammingException, QCoreApplication::applicationName() + " parameter '" + data.name + "' declared twice!");
	}

    parameters_.append(data);
}

int ToolBase::checkParameterExists(QString name, ParameterType type) const
{
    int index = parameterIndex(name);
    if (index==-1)
	{
		THROW(ProgrammingException, QCoreApplication::applicationName() + " parameter '" + name + "' not declared, but used!");
	}

    if (parameters_[index].type!=type)
	{
        THROW(ProgrammingException, QCoreApplication::applicationName() + " parameter '" + name + "' is expected to have type '" + typeToString(type) + "', but has type '" + typeToString(parameters_[index].type) + "'!");
	}

    return index;
}

QString ToolBase::typeToString(ToolBase::ParameterType type) const
{
	if (type==NONE)
	{
		return "NONE";
	}
	else if (type==FLAG)
	{
		return "FLAG";
	}
	else if (type==INT)
	{
		return "INT";
	}
	else if (type==FLOAT)
	{
		return "FLOAT";
	}
	else if (type==ENUM)
	{
		return "ENUM";
	}
	else if (type==STRING)
	{
		return "STRING";
	}
	else if (type==INFILE)
	{
		return "INFILE";
	}
	else if (type==OUTFILE)
	{
		return "OUTFILE";
	}
	else if (type==INFILELIST)
	{
		return "INFILELIST";
	}

	THROW(ProgrammingException, "Unknown ToolBase parameter type!");
}

QString ToolBase::typeToArgString(ToolBase::ParameterType type) const
{
	if (type==FLAG)
	{
		return "";
	}
	else if (type==INT)
	{
		return "<int>";
	}
	else if (type==FLOAT)
	{
		return "<float>";
	}
	else if (type==ENUM)
	{
		return "<enum>";
	}
	else if (type==STRING)
	{
		return "<string>";
	}
	else if (type==INFILE)
	{
		return "<file>";
	}
	else if (type==OUTFILE)
	{
		return "<file>";
	}
	else if (type==INFILELIST)
	{
		return "<filelist>";
	}

	THROW(ProgrammingException, "Unknown ToolBase parameter type!");
}

ToolBase::ParameterData::ParameterData()
    : name()
    , type(NONE)
	, desc("Invalid uninitialized parameter")
	, optional(false)
	, default_value("Schwenker")
	, value()
{
}

ToolBase::ParameterData::ParameterData(QString n, ParameterType t, QString d, bool o, QVariant v)
    : name (n)
    , type(t)
	, desc(d)
	, optional(o)
	, default_value(v)
	, value()
{
}

bool ToolBase::notify(QObject* receiver, QEvent* event)
{
	try
	{
		return QCoreApplication::notify(receiver, event);
	}
	catch(CommandLineParsingException& e)
	{
		QTextStream stream(stderr);
		stream << "Command line parsing exception: " << e.message() << endl;
		stream << "Call this tool with the argument '--help' for help." << endl;
		exit(1);
	}
	catch(ToolFailedException& e)
	{
		QTextStream stream(stderr);
		stream << e.message() << endl;
		exit(1);
	}
	catch(ProgrammingException& e)
	{
		QTextStream stream(stderr);
		stream << "Programming exception: " << e.message() << endl;
		stream << "Location             : " << e.file() << ":" << e.line() << endl;
		stream << "This should not happen, please report the error to the developers!" << endl;
		exit(1);
	}
	catch(Exception& e)
	{
		QTextStream stream(stderr);
		stream << "Exception: " << e.message() << endl;
		stream << "Location : " << e.file() << ":" << e.line() << endl;
		exit(1);
	}
	catch(std::exception& e)
	{
		QTextStream stream(stderr);
		stream << "Exception: " << e.what() << endl;
		exit(1);
	}
	catch(...)
	{
		QTextStream stream(stderr);
		stream << "Unknown exception!" << endl;
		exit(1);
	}

	return false;
}
