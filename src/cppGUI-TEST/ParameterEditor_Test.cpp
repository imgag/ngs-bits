#include "ParameterEditor.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>
#include <limits>

class ParameterEditor_Test : public QObject
{
	Q_OBJECT

private:
	QList<Parameter> definitions() const
	{
		return {
			Parameter("count", "Count", ParameterType::INT, {{ConstraintType::MIN, 1.5}, {ConstraintType::MAX, 10}}),
			Parameter("scale", "Scale", ParameterType::DOUBLE, {{ConstraintType::MIN, -1.0}, {ConstraintType::MAX, 1.0}}),
			Parameter("enabled", "Enabled", ParameterType::BOOL, {}),
			Parameter("mode", "Mode", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("A\tB")}}),
			Parameter("text", "Text", ParameterType::STRING, {{ConstraintType::NON_EMPTY, true}}),
			Parameter("entries", "Entries", ParameterType::STRINGLIST, {}),
			Parameter("choices", "Choices", ParameterType::STRINGLIST, {{ConstraintType::ALLOWED_VALUES, QByteArray("A\tB")}, {ConstraintType::NON_EMPTY, true}})
		};
	}

	QHash<QByteArray, QVariant> defaults() const
	{
		return {{"count", 3}, {"scale", 0.12345678901234566}, {"enabled", true}, {"mode", QByteArray("A")},
			{"text", QByteArray("loaded")}, {"entries", QVariant::fromValue(QByteArrayList({"", "A", "A", "line\nbreak"}))},
			{"choices", QVariant::fromValue(QByteArrayList({"A"}))}};
	}

private slots:
	void loadAndStore()
	{
		ParameterList parameters("test", definitions(), defaults());
		ParameterEditor editor(parameters);
		QSignalSpy changes(&parameters, &ParameterList::parameterChanged);
		QVERIFY(editor.store());
		QCOMPARE(changes.count(), 0);
		QCOMPARE(parameters.getStringList("entries"), defaults().value("entries").value<QByteArrayList>());
		QCOMPARE(parameters.getDouble("scale"), defaults().value("scale").toDouble());
		editor.findChild<QLineEdit*>("count")->setText("10");
		editor.findChild<QLineEdit*>("scale")->setText("-0.75");
		editor.findChild<QCheckBox*>("enabled")->setChecked(false);
		editor.findChild<QComboBox*>("mode")->setCurrentIndex(1);
		editor.findChild<QLineEdit*>("text")->setText(QString::fromUtf8("Grüße"));
		QCOMPARE(parameters.getInt("count"), 3);
		QVERIFY(editor.store());
		QCOMPARE(parameters.getInt("count"), 10);
		QCOMPARE(parameters.getDouble("scale"), -0.75);
		QCOMPARE(parameters.getBool("enabled"), false);
		QCOMPARE(parameters.getString("mode"), QByteArray("B"));
		QCOMPARE(parameters.getString("text"), QString::fromUtf8("Grüße").toUtf8());
		QCOMPARE(changes.count(), 1);
		editor.reset();
		QCOMPARE(parameters.getInt("count"), 10);
		QVERIFY(editor.store());
		QCOMPARE(parameters.getInt("count"), 3);
		QCOMPARE(parameters.getDouble("scale"), defaults().value("scale").toDouble());
	}

	void invalidInputDoesNotPartiallyApply()
	{
		ParameterList parameters("test", definitions(), defaults());
		ParameterEditor editor(parameters);
		editor.findChild<QLineEdit*>("text")->setText("pending");
		for (const QString& invalid : {"", "1", "11", "2.5", "nan", "2147483648", "abc"})
		{
			editor.findChild<QLineEdit*>("count")->setText(invalid);
			QVERIFY(!editor.store());
			QVERIFY(!editor.findChild<QPushButton*>("apply")->isEnabled());
			QCOMPARE(parameters.getInt("count"), 3);
			QCOMPARE(parameters.getString("text"), QByteArray("loaded"));
		}
		editor.findChild<QLineEdit*>("count")->setText("2");
		editor.findChild<QLineEdit*>("text")->clear();
		QVERIFY(!editor.store());
		editor.findChild<QLineEdit*>("text")->setText("pending");
		editor.findChild<QLineEdit*>("scale")->setText("1.00001");
		QVERIFY(!editor.store());
		editor.findChild<QLineEdit*>("scale")->setText("1");
		QVERIFY(editor.store());
	}

	void autoApplyAndReset()
	{
		ParameterList parameters("test", definitions(), defaults());
		ParameterEditor editor(parameters);
		editor.findChild<QLineEdit*>("count")->setText("invalid");
		editor.findChild<QLineEdit*>("text")->setText("pending");
		editor.findChild<QCheckBox*>("auto_apply")->setChecked(true);
		QCOMPARE(parameters.getString("text"), QByteArray("pending"));
		QCOMPARE(parameters.getInt("count"), 3);
		editor.findChild<QLineEdit*>("count")->setText("8");
		QCOMPARE(parameters.getInt("count"), 8);
		editor.reset();
		QCOMPARE(parameters.getInt("count"), 3);
		QCOMPARE(parameters.getString("text"), QByteArray("loaded"));
		editor.findChild<QCheckBox*>("auto_apply")->setChecked(false);
		editor.findChild<QLineEdit*>("count")->setText("9");
		QCOMPARE(parameters.getInt("count"), 3);
	}

	void stringLists()
	{
		ParameterList parameters("test", definitions(), defaults());
		ParameterEditor editor(parameters);
		auto* entries = editor.findChild<QWidget*>("entries")->findChild<QListWidget*>("entries");
		QCOMPARE(entries->count(), 4);
		entries->item(0)->setText("changed");
		QVERIFY(editor.store());
		QCOMPARE(parameters.getStringList("entries").first(), QByteArray("changed"));
		auto* choices = editor.findChild<QWidget*>("choices")->findChild<QListWidget*>("entries");
		choices->item(0)->setText("C");
		QVERIFY(!editor.store());
		choices->item(0)->setText("");
		QVERIFY(!editor.store());
		choices->item(0)->setText("B");
		QVERIFY(editor.store());
		QCOMPARE(parameters.getStringList("choices"), QByteArrayList({"B"}));
		entries->clear();
		QVERIFY(editor.store());
		QVERIFY(parameters.getStringList("entries").isEmpty());
		editor.reset();
		QVERIFY(editor.store());
		QCOMPARE(parameters.getStringList("entries"), defaults().value("entries").value<QByteArrayList>());
	}

	void rebindAndConstraintChanges()
	{
		ParameterList first("first", definitions(), defaults());
		ParameterList second("second", {Parameter("other", "Other", ParameterType::INT, {})}, {{"other", std::numeric_limits<int>::min()}});
		ParameterEditor editor(first);
		first.overrideConstraint("count", ConstraintType::MAX, 4);
		editor.findChild<QLineEdit*>("count")->setText("5");
		QVERIFY(!editor.store());
		editor.load(second);
		QVERIFY(!editor.findChild<QLineEdit*>("count"));
		QVERIFY(editor.store());
		QCOMPARE(second.getInt("other"), std::numeric_limits<int>::min());
		first.overrideConstraint("count", ConstraintType::MAX, 6);
		QVERIFY(editor.findChild<QLineEdit*>("other"));
		second.reset({}, {});
		QVERIFY(!editor.findChild<QLineEdit*>("other"));
		QVERIFY(editor.store());
	}

	void destroyedList()
	{
		auto* parameters = new ParameterList("test", definitions(), defaults());
		ParameterEditor editor(*parameters);
		delete parameters;
		QVERIFY(!editor.isEnabled());
		QVERIFY(!editor.store());
		editor.reset();
	}
};

QTEST_MAIN(ParameterEditor_Test)
#include "ParameterEditor_Test.moc"
