#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QList>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QSharedPointer>
#include <QProcess>
#include <QDebug>
#include <QMetaObject>
#include <QMetaMethod>
#include <QRegularExpression>
#include <cmath>
#include "Exceptions.h"
#include "Helper.h"
#include "VersatileFile.h"

namespace TFW
{
    class TestExecutor
    {
    public:
        TestExecutor()
        {
        }

        const QByteArray& name() const
        {
            return name_;
        }
        void setName(const QByteArray& name)
        {
            name_ = name;
        }

        int testCount()
        {
            return tests_.count();
        }
        QByteArray methodName(int i)
        {
            return tests_[i].first;
        }
        std::function<void()> method(int i)
        {
            return tests_[i].second;
        }

    protected:
        QByteArray name_;
        QList<QPair<QByteArray, std::function<void()>>> tests_;
    };

	//############## helper functions ##################

	inline QByteArray name(QString filename)
	{
		return QFileInfo(filename).fileName().toUtf8();
	}

	inline QByteArray number(int num)
	{
		return QByteArray::number(num);
	}

	inline bool eq(int a, int e)
	{
		return a==e;
	}

	inline bool eq(double a, double e)
	{
		return fabs(a-e)<0.00001;
	}

	inline bool eq(QString a, QString e)
	{
		return a==e;
	}

	inline QByteArray findTestDataFile(QByteArray sourcefile, QByteArray testfile)
	{
        QString bin_folder = QFileInfo(sourcefile).absoluteDir().currentPath();
        QString root_folder = QFileInfo(bin_folder).absolutePath();

        QList<QByteArray> src_path_parts = sourcefile.split('/');
        QString project_folder = sourcefile;
        if (src_path_parts.size()>1)
        {
            project_folder = src_path_parts[src_path_parts.size()-2];
        }

        QString final_path;
        if (testfile.startsWith("out/"))
        {
            final_path = bin_folder + "/" +testfile;
        }
        else
        {
            final_path = root_folder + "/src/" + project_folder + "/" + testfile;
        }

        if (!QFile::exists(final_path)) THROW(ProgrammingException, "Could not find test file '" + testfile + "'!");
        return final_path.toLatin1();
	}

	//################## status variables ####################

	inline bool& skipped()
	{
		static bool skipped = false;
		return skipped;
	}

	inline bool& failed()
	{
		static bool failed = false;
		return failed;
	}

	inline QByteArray& message()
	{
		static QByteArray message;
		return message;
	}

	//############### test execution ##################

    inline QList<TestExecutor*>& testList()
	{
        static QList<TestExecutor*> list;
		return list;
	}

    inline void addTest(TestExecutor* test)
    {
        QList<TestExecutor*>& list = testList();
        foreach (TestExecutor* test2, list)
		{
            if (test2->name()==test->name()) return;
		}
        list.append(test);
	}

	inline int run(int argc, char *argv[])
    {
		//create a QCoreApplication to be able to use a event loop (e.g. for XML validation)
		QCoreApplication core_app(argc, argv);

		//parse command line parameters
		QCommandLineParser parser;
		parser.addOption(QCommandLineOption("s", "Test case string filter for test cases.", "s"));
		parser.addOption(QCommandLineOption("l", "Test case list to execute.", "l"));
		parser.addOption(QCommandLineOption("d", "Enable debug output."));
		parser.addHelpOption();
		parser.process(core_app);
		QByteArray s_filter = parser.value("s").toUtf8();
		QStringList l_filter;
		if (parser.value("l")!="")
		{
			try
			{
				l_filter = Helper::loadTextFile(parser.value("l"), true, '#', true);
			}
			catch(Exception e)
			{
				qDebug() << "Error loading filter list " << parser.value("l");
				qDebug() << e.message();
				return -1;
			}
		}
		bool debug_output = parser.isSet("d");

		//create folder for test output data
		QDir(".").mkdir("out");

		//open output stream
		QFile outstream;
		outstream.open(stdout, QFile::WriteOnly);

		//run tests
		QElapsedTimer timer_all;
		timer_all.start();
		QElapsedTimer timer;
		int c_failed = 0;
		int c_skipped = 0;
        int c_passed = 0;
        foreach (TestExecutor* test, testList())
		{
            QByteArray test_name = test->name();

			//delete output files of previous test runs
			QStringList old_files = Helper::findFiles("out/", test_name.left(test_name.length()-5)+"*.*", false);
			foreach(QString old_file, old_files)
			{
				QFileInfo file_info(old_file);
				if(file_info.isFile() && file_info.exists())
				{
					QFile::remove(old_file);
				}
			}

            for (int i=0; i<test->testCount(); ++i)
			{
                std::function<void()> method = test->method(i);
                QByteArray method_name = test->methodName(i);
                QByteArray test_and_method = test_name + "::" + method_name;

                //string filter
				if (!test_and_method.contains(s_filter))
				{
					continue;
				}

				//test list filter
				if (!l_filter.isEmpty())
				{
					bool found = false;
					foreach(QString filter, l_filter)
					{
						if (test_and_method+"()"==filter || test_and_method==filter)
						{
							found = true;
						}
					}
					if (!found) continue;
				}

				//execute test
				skipped() = false;
				failed() = false;
                message() = "";
				timer.restart();
				try
				{
					if (debug_output)
					{
                        outstream.write("Performing " + test_name + ":" + method_name + "\n");
						outstream.flush();
					}
				}
				catch (Exception& e)
				{
					QByteArray msg;
					msg += "exception: Exception (cppCORE)\n";
					msg += "location : " + name(e.file()) + ":" + QByteArray::number(e.line()) + "\n";
					msg += "message  : " + e.message().toUtf8() + "\n";
					message() = msg;
					failed() = true;
				}
				catch (std::exception& e)
				{
					QByteArray msg;
					msg += "exception: std::exception\n";
					msg += "message  : " + QByteArray(e.what()) + "\n";
					message() = msg;
					failed() = true;
				}
				catch (...)
				{
					message() = "unknown exception";
					failed() = true;
                }

				//evaluate what happened
				QByteArray result;
				if (failed())
				{
					result = "FAIL!";
					++c_failed;
				}
				else if(skipped())
				{
					result = "SKIP";
					++c_skipped;
				}
				else
				{
					result = "PASS";
					++c_passed;
				}

				//output
				outstream.write(result + "\t" + test_and_method + "()\t" + Helper::elapsedTime(timer, true) + "\n");
				if (!message().isEmpty())
				{
					QList<QByteArray> parts = message().trimmed().split('\n');
					foreach(QByteArray part, parts)
					{
						outstream.write("  " + part.trimmed() + "\n");
					}
				}
				outstream.flush();
			}
		}

		outstream.write("\n");
		outstream.write("PASSED : " + QByteArray::number(c_passed).rightJustified(3, ' ') + "\n");
		outstream.write("SKIPPED: " + QByteArray::number(c_skipped).rightJustified(3, ' ') + "\n");
		outstream.write("FAILED : " + QByteArray::number(c_failed).rightJustified(3, ' ') + "\n");
		outstream.write("TIME   : " + Helper::elapsedTime(timer_all, true) + "\n");
		outstream.close();

		return c_failed;
	}

	///Executes a tool and returns (1) if the execution was successful (2) the error message if it was not successful
	inline QString executeTool(QString toolname, QString arguments, bool ignore_error_code, QString file, int line)
	{
		if (QFile::exists(toolname)) //Linux
		{
			toolname = "./" + toolname;
		}
		else if (QFile::exists(toolname + ".exe")) //Windows
		{
			toolname = toolname + ".exe";
		}
		else
		{
			return "Tool '" + toolname + "' not found!";
		}

		QProcess process;
		QString log_file = "out/" + QFileInfo(file).baseName() + "_line" + QString::number(line) + ".log";
		process.setProcessChannelMode(QProcess::MergedChannels);
		process.setStandardOutputFile(log_file);
		QStringList arg_split = arguments.split(' ');
		for(int i=0; i<arg_split.count(); ++i)
		{
			arg_split[i].replace("%20", " ");
		}
		process.start(toolname, arg_split);
		bool started = process.waitForStarted(-1);
		bool finished = process.waitForFinished(-1);
		int exit_code = process.exitCode();
		if (!started || !finished || (!ignore_error_code && exit_code!=0))
		{
			QByteArray result = "exit code: " + QByteArray::number(exit_code);
			QFile tmp_file(log_file);
			tmp_file.open(QFile::ReadOnly|QFile::Text);
			result += "\ntool output:\n" + tmp_file.readAll().trimmed();
			return result;
		}
		return "";
	}

	/**
	 * @brief comareFiles
	 * Compares files line by line to check if they are identical, but uses a delta to check numerics
	 * @param actual
	 * @param expected
	 * @param delta How much absolute deviation should be allowed for numeric values. If delta is 0 then it will not be considered.
	 * @param delta_is_percentage If 'true', the delta is interpreted as percentage in the range [0-100] instead of as absolute value.
	 * @return empty string on success, otherwise return the diff
	 */
	inline QString comareFiles(QString actual, QString expected, double delta, bool delta_is_percentage, char separator)
	{
	   actual = QFileInfo(actual).absoluteFilePath();
	   expected = QFileInfo(expected).absoluteFilePath();

		//open files
		QFile afile(actual);
		if (!afile.open(QIODevice::ReadOnly | QIODevice::Text)) return "Could not open actual file '" + actual.toUtf8() + " for reading!";
		QFile efile(expected);
		if (!efile.open(QIODevice::ReadOnly | QIODevice::Text)) return "Could not open expected file '" + expected.toUtf8() + " for reading!";

		//compare lines
		int line_nr = 1;
		QTextStream astream(&afile);
		QTextStream estream(&efile);
		while (!astream.atEnd() && !estream.atEnd())
		{
			QString aline = astream.readLine();
			QString eline = estream.readLine();
			if(aline!=eline)
			{
				//not delta allowed > no numeric comparison
				if (delta == 0.0)
				{
					return "Differing line "  + QByteArray::number(line_nr) + "\nactual   : " + aline + "\nexpected : " + eline;
				}

				//numeric comparison
				QStringList a_line_items = aline.split(separator);
				QStringList e_line_items = eline.split(separator);
				if (a_line_items.size() != e_line_items.size())
				{
					return "Differing line "  + QByteArray::number(line_nr) + " (different token count)\nactual   : " + aline + "\nexpected : " + eline;
				}

				for (int i=0; i<a_line_items.size(); ++i)
				{
					if (a_line_items[i]!=e_line_items[i])
					{
						bool a_item_is_numeric;
						float a_line_value = a_line_items[i].toFloat(&a_item_is_numeric);

						bool e_item_is_numeric;
						float e_line_value = e_line_items[i].toFloat(&e_item_is_numeric);

						if (!a_item_is_numeric || !e_item_is_numeric)
						{
							return "Differing line "  + QByteArray::number(line_nr) + " (non-numeric difference)\nactual   : " + aline + "\nexpected : " + eline;
						}

						double abs_diff = fabs(a_line_value-e_line_value);
						if (delta_is_percentage)
						{
							double rel_diff = fabs(a_line_value-e_line_value)/e_line_value;
							if (rel_diff > delta/100.0)
							{
								return "Differing numeric value in line "  + QByteArray::number(line_nr) + " (relative difference too big)\nactual   : " + QString::number(a_line_value) + "\nexpected : " + QString::number(e_line_value) + "\ndelta rel: " + QString::number(rel_diff, 'g', 4);
							}
						}
						else
						{
							if (abs_diff > delta)
							{
								return "Differing numeric value in line "  + QByteArray::number(line_nr) + " (absolute difference too big)\nactual   : " + QString::number(a_line_value) + "\nexpected : " + QString::number(e_line_value)+ "\ndelta abs: " + QString::number(abs_diff, 'g', 4);
							}
						}
					}
				}
			}
			++line_nr;
		}

		//compare rest (ignore lines containing only whitespaces)
		QString arest = astream.readAll().trimmed();
		if (!arest.isEmpty()) return "Actual file '" + actual + "' contains more data than expected file '" + expected + "': " + arest;
		QString erest = estream.readAll().trimmed();
		if (!erest.isEmpty()) return "Expected file '" + expected + "' contains more data than actual file '" + actual + "': " + erest;

		return "";
	}

	inline QString comareFilesGZ(QString actual, QString expected)
	{
		//make file names absolute
		actual = QFileInfo(actual).absoluteFilePath();
		expected = QFileInfo(expected).absoluteFilePath();

		//open streams
		VersatileFile streama(actual);
		streama.open();
		VersatileFile streame(actual);
		streame.open();

		//compare lines
		int line_nr = 1;
		while (!streama.atEnd() && !streame.atEnd())
		{
			QByteArray aline = streama.readLine(true);
			QByteArray eline = streame.readLine(true);
			if (eline!=aline)
			{
				return "Differing line "  + QByteArray::number(line_nr) + "\nactual   : " + aline + "\nexpected : " + eline;
			}
			++line_nr;
		}

		//check if line counts differ
		if (!streama.atEnd()) return "Actual file '" + actual + "' has more lines than expected file '" + expected + "'!";
		if (!streame.atEnd()) return "Actual file '" + actual + "' has less lines than expected file '" + expected + "'!";

		return "";
	}

	inline QString removeLinesMatching(QString filename, QRegularExpression regexp)
	{
		QFile file(filename);
		QList<QByteArray> output;

		//read input
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "Could not open file '" + filename + " for reading!";
		while(!file.atEnd())
		{
			QByteArray line = file.readLine();
			if (regexp.match(line).hasMatch()) continue;
			output.append(line);
		}
		file.close();

		//store output
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return "Could not open file '" + filename + " for writing!";
		foreach(const QByteArray& line, output)
		{
			file.write(line);
		}
		file.close();

		return "";
	}

	/// Helper class to create a test instance and add it to the test list
	template <class T>
	class TestCreator
	{
	public:
		TestCreator(QByteArray name)
		{
			//check name
			if (!name.endsWith("_Test"))
			{
				qDebug() << "Test name must end with '_Test', but does not: " + name;
			}

			//add tests
			TestExecutor* inst = new T();
			inst->setName(name);
			addTest(inst);
		}
	};

} //namespace

#define TEST_CLASS(className) \
    class className; \
    using TestClassType = className; \
    static TFW::TestCreator<className> t(#className); \
    class className : public TFW::TestExecutor

#define TEST_METHOD(methodName) \
	struct Register_##methodName { \
        Register_##methodName(TestClassType* obj) { \
			obj->tests_.push_back({#methodName, [obj]{ obj->methodName(); }}); \
		} \
} reg_##methodName{this}; \
	void methodName()

#define SKIP(msg)\
{\
	TFW::skipped() = true;\
	TFW::message() = QByteArray("message  : ") + #msg + "\n"\
	+ "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
	return;\
}

//##################### simple comparison macros #####################

#define IS_TRUE(stmt)\
	if ((stmt)==false)\
	{\
		TFW::failed() = true;\
		TFW::message() = "IS_TRUE(" + QByteArray(#stmt) + ") failed\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}

#define IS_FALSE(stmt)\
	if ((stmt)==true)\
	{\
		TFW::failed() = true;\
		TFW::message() = "IS_FALSE(" + QByteArray(#stmt) + ") failed\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}

#define I_EQUAL(actual, expected)\
	if (actual!=expected)\
	{\
		TFW::failed() = true;\
		TFW::message() = "I_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "actual   : " + QByteArray::number((qlonglong)actual) + "\n"\
					   + "expected : " + QByteArray::number((qlonglong)expected) + "\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}


#define F_EQUAL(actual, expected)\
	if (fabs(actual-expected)>0.00000001)\
	{\
		TFW::failed() = true;\
		TFW::message() = "F_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "actual   : " + QByteArray::number(actual) + "\n"\
					   + "expected : " + QByteArray::number(expected) + "\n"\
					   + "max delta: " + QByteArray::number(0.00000001) + "\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}

#define F_EQUAL2(actual, expected, delta)\
	if (fabs(actual-expected)>delta)\
	{\
		TFW::failed() = true;\
		TFW::message() = "F_EQUAL2(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "actual   : " + QByteArray::number(actual) + "\n"\
					   + "expected : " + QByteArray::number(expected) + "\n"\
					   + "max delta: " + QByteArray::number(delta) + "\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}

#define S_EQUAL(actual, expected)\
	if (QString(actual)!=QString(expected))\
	{\
		TFW::failed() = true;\
		TFW::message() = "S_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "actual   : " + QString(actual).toUtf8() + "\n"\
					   + "expected : " + QString(expected).toUtf8() + "\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}

#define X_EQUAL(actual, expected)\
	if (!(actual==expected))\
	{\
		TFW::failed() = true;\
		TFW::message() = "X_EQUAL(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
					   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__);\
		return;\
	}

#define IS_THROWN(exception_type, command)\
	{\
		bool thrown = false;\
		bool right = false;\
		try{ command; } catch( exception_type e) { thrown=true; right=true; } catch(...) { thrown=true; } \
		if (!thrown)\
		{\
			TFW::failed() = true;\
			TFW::message() = "IS_THROWN(" + QByteArray(#exception_type) + ", " + QByteArray(#command) + ") failed\n"\
						  + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						  + "message  : no exception thrown";\
			return;\
		}\
		if (!right)\
		{\
			TFW::failed() = true;\
			TFW::message() = "IS_THROWN(" + QByteArray(#exception_type) + ", " + QByteArray(#command) + ") failed\n"\
						  + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						  + "message  : exception thrown, but not a '" + QByteArray(#exception_type) + "'";\
			return;\
		}\
	}
//##################### tool execution and file comparison macros #####################

#define EXECUTE(toolname, arguments) \
	{\
		QString tfw_result = TFW::executeTool(toolname, arguments, false, __FILE__, __LINE__);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "EXECUTE(" + QByteArray(#toolname) + ", " + QByteArray(#arguments) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toUtf8();\
			return;\
		}\
	}

//Allows execute without propagating the error. Doesn't check for/enforce return code 1.
#define EXECUTE_FAIL(toolname, arguments) \
	{\
		QString tfw_result = TFW::executeTool(toolname, arguments, true, __FILE__, __LINE__);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "EXECUTE_FAIL(" + QByteArray(#toolname) + ", " + QByteArray(#arguments) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toUtf8();\
			return;\
		}\
	}

#define COMPARE_FILES(actual, expected)\
	{\
		QString tfw_result = TFW::comareFiles(actual, expected, 0.0, true, '\t');\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "COMPARE_FILES(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toUtf8();\
			return;\
		}\
	}

#define COMPARE_FILES_DELTA(actual, expected, delta, delta_is_percentage, separator)\
		{\
				QString tfw_result = TFW::comareFiles(actual, expected, delta, delta_is_percentage, separator);\
				if (tfw_result!="")\
				{\
						TFW::failed() = true;\
						TFW::message() = "COMPARE_FILES_DELTA(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ", " + QByteArray(#delta) + ", " + QByteArray(#delta_is_percentage) + ", " + QByteArray(#separator) + ") failed\n"\
												   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
												   + "message  : " + tfw_result.toUtf8();\
						return;\
				}\
		}

#define COMPARE_GZ_FILES(actual, expected)\
	{\
		QString tfw_result = TFW::comareFilesGZ(actual, expected);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "COMPARE_GZ_FILES(" + QByteArray(#actual) + ", " + QByteArray(#expected) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toUtf8();\
			return;\
		}\
	}

#define REMOVE_LINES(filename, regexp)\
	{\
		QString tfw_result = TFW::removeLinesMatching(filename, regexp);\
		if (tfw_result!="")\
		{\
			TFW::failed() = true;\
			TFW::message() = "REMOVE_LINES(" + QByteArray(#filename) + ", " + QByteArray(#regexp) + ") failed\n"\
						   + "location : " + TFW::name(__FILE__) + ":" + TFW::number(__LINE__) + "\n"\
						   + "message  : " + tfw_result.toUtf8();\
			return;\
		}\
	}

#define TESTDATA(filename)\
	 TFW::findTestDataFile(__FILE__, filename)

#endif // TESTFRAMEWORK_H
