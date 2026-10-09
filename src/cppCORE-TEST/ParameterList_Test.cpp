#include "TestFramework.h"
#include "ParameterList.h"
#include <limits>

TEST_CLASS(ParameterList_Test)
{
private:
	static Parameter config()
	{
		Parameter output("test_track");
		output.append(ParameterDescription("height", "Height in pixels", ParameterType::INT, {{ConstraintType::MIN, 1}, {ConstraintType::MAX, 100}}));
		output.append(ParameterDescription("scale", "Plot scale", ParameterType::FLOAT, {{ConstraintType::MIN, -0.5}, {ConstraintType::MAX, 0.5}}));
		output.append(ParameterDescription("visible", {}, ParameterType::BOOL, {}));
		output.append(ParameterDescription("mode", {}, ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, QByteArray("POINTS\tHEATMAP")}}));
		return output;
	}

	static QHash<QByteArray, QVariant> defaults()
	{
		return {{"height", 10}, {"scale", 0.0}, {"visible", true}, {"mode", QByteArray("POINTS")}};
	}

	TEST_METHOD(metadata)
	{
		Parameter metadata = config();
		S_EQUAL(metadata.trackName(), "test_track");
		const ParameterDescription height = metadata.parameter("height");
		S_EQUAL(height.name(), "height");
		S_EQUAL(height.description(), "Height in pixels");
		IS_TRUE(height.type() == ParameterType::INT);
		I_EQUAL(height.constraints().value(ConstraintType::MIN).toInt(), 1);
		I_EQUAL(metadata.parameters().size(), 4);
		S_EQUAL(metadata.parameters()[0].name(), "height");
		S_EQUAL(metadata.parameters()[3].name(), "mode");
		IS_THROWN(ArgumentException, metadata.parameter("unknown"));
		IS_THROWN(ArgumentException, metadata.append(height));
		metadata.clear();
		I_EQUAL(metadata.parameters().size(), 0);
		IS_THROWN(ArgumentException, metadata.parameter("height"));
		metadata.append(height);
		I_EQUAL(metadata.parameters().size(), 1);
	}

	TEST_METHOD(enumStrings)
	{
		S_EQUAL(ParameterDescription::toString(ParameterType::INT), "INT");
		S_EQUAL(ParameterDescription::toString(ParameterType::FLOAT), "FLOAT");
		S_EQUAL(ParameterDescription::toString(ParameterType::BOOL), "BOOL");
		S_EQUAL(ParameterDescription::toString(ParameterType::STRING), "STRING");
		S_EQUAL(ParameterDescription::toString(ParameterType::STRINGLIST), "STRINGLIST");
		S_EQUAL(ParameterDescription::toString(ConstraintType::MIN), "MIN");
		S_EQUAL(ParameterDescription::toString(ConstraintType::MAX), "MAX");
		S_EQUAL(ParameterDescription::toString(ConstraintType::ALLOWED_VALUES), "ALLOWED_VALUES");
		S_EQUAL(ParameterDescription::toString(ConstraintType::NON_EMPTY), "NON_EMPTY");
		IS_THROWN(ProgrammingException, ParameterDescription::toString(static_cast<ParameterType>(-1)));
		IS_THROWN(ProgrammingException, ParameterDescription::toString(static_cast<ConstraintType>(-1)));
	}

	TEST_METHOD(invalidConstraints)
	{
		for (ParameterType type : {ParameterType::INT, ParameterType::FLOAT})
		{
			IS_THROWN(ArgumentException, ParameterDescription("number", {}, type, {{ConstraintType::MIN, 10}, {ConstraintType::MAX, 5}}));
			for (ConstraintType key : {ConstraintType::MIN, ConstraintType::MAX})
			{
				IS_THROWN(ArgumentException, ParameterDescription("number", {}, type, {{key, QByteArray("invalid")}}));
				IS_THROWN(ArgumentException, ParameterDescription("number", {}, type, {{key, QByteArray(400, '9')}}));
				IS_THROWN(ArgumentException, ParameterDescription("number", {}, type, {{key, -std::numeric_limits<double>::infinity()}}));
			}
			IS_THROWN(ArgumentException, ParameterDescription("number", {}, type, {{ConstraintType::ALLOWED_VALUES, QByteArray("1\t2")}}));
		}
		IS_THROWN(ArgumentException, ParameterDescription("flag", {}, ParameterType::BOOL, {{ConstraintType::MIN, 0}}));
		IS_THROWN(ArgumentException, ParameterDescription("text", {}, ParameterType::STRING, {{ConstraintType::MAX, 1}}));
	}

	TEST_METHOD(defaultNames)
	{
		Parameter metadata = config();
		for (const QByteArray& name : defaults().keys())
		{
			auto missing = defaults();
			missing.remove(name);
			IS_THROWN(ArgumentException, ParameterList(metadata, missing));
		}
		auto extra = defaults();
		extra.insert("unknown", 1);
		IS_THROWN(ArgumentException, ParameterList(metadata, extra));
		extra.remove("height");
		IS_THROWN(ArgumentException, ParameterList(metadata, extra));
		Parameter empty("empty");
		ParameterList empty_parameters(empty, {});
		IS_THROWN(ArgumentException, ParameterList(empty, extra));
	}

	TEST_METHOD(defaultNormalization)
	{
		auto initial = defaults();
		initial["height"] = QByteArray("25");
		initial["scale"] = QString("0.25");
		initial["mode"] = QString("HEATMAP");
		ParameterList parameters(config(), initial);
		I_EQUAL(parameters.getInt("height"), 25);
		F_EQUAL(parameters.getFloat("scale"), 0.25);
		IS_TRUE(parameters.getBool("visible"));
		S_EQUAL(parameters.getString("mode"), "HEATMAP");
		int changes = 0;
		QObject::connect(&parameters, &ParameterList::parameterChanged, [&changes]() { ++changes; });
		parameters.setInt("height", 25);
		parameters.setFloat("scale", 0.25);
		parameters.setBool("visible", true);
		parameters.setString("mode", "HEATMAP");
		I_EQUAL(changes, 0);
	}

	TEST_METHOD(invalidDefaults)
	{
		for (const QByteArray& name : defaults().keys())
		{
			auto initial = defaults();
			initial[name] = QVariant();
			IS_THROWN(ArgumentException, ParameterList(config(), initial));
		}
		const QList<QPair<QByteArray, QVariant>> invalid_values{
			{"height", 0}, {"height", 101}, {"height", 2.7}, {"height", 1e20}, {"height", QByteArray("abc")},
			{"scale", -0.6}, {"scale", 0.6}, {"scale", QByteArray("abc")},
			{"scale", std::numeric_limits<double>::quiet_NaN()}, {"scale", std::numeric_limits<double>::infinity()},
			{"visible", QString("false")}, {"visible", 1}, {"mode", 1}, {"mode", QByteArray("UNKNOWN")}
		};
		for (const auto& invalid : invalid_values)
		{
			auto initial = defaults();
			initial[invalid.first] = invalid.second;
			IS_THROWN(ArgumentException, ParameterList(config(), initial));
		}
	}

	TEST_METHOD(settersAndSignals)
	{
		ParameterList parameters(config(), defaults());
		int changes = 0;
		int observed_height = 0;
		QObject::connect(&parameters, &ParameterList::parameterChanged, [&]()
		{
			++changes;
			observed_height = parameters.getInt("height");
		});
		parameters.setInt("height", 1);
		I_EQUAL(observed_height, 1);
		parameters.setInt("height", 100);
		parameters.setFloat("scale", -0.5);
		parameters.setFloat("scale", 0.5);
		parameters.setBool("visible", false);
		parameters.setString("mode", "HEATMAP");
		I_EQUAL(changes, 6);
		parameters.setInt("height", 100);
		parameters.setFloat("scale", 0.5);
		parameters.setBool("visible", false);
		parameters.setString("mode", "HEATMAP");
		I_EQUAL(changes, 6);
		IS_THROWN(ArgumentException, parameters.setInt("height", 0));
		IS_THROWN(ArgumentException, parameters.setInt("height", 101));
		IS_THROWN(ArgumentException, parameters.setFloat("scale", 0.6));
		IS_THROWN(ArgumentException, parameters.setFloat("scale", std::numeric_limits<double>::infinity()));
		IS_THROWN(ArgumentException, parameters.setFloat("scale", std::numeric_limits<double>::quiet_NaN()));
		IS_THROWN(ArgumentException, parameters.setString("mode", "heatmap"));
		I_EQUAL(changes, 6);
		I_EQUAL(parameters.getInt("height"), 100);
		F_EQUAL(parameters.getFloat("scale"), 0.5);
		IS_FALSE(parameters.getBool("visible"));
		S_EQUAL(parameters.getString("mode"), "HEATMAP");
	}

	TEST_METHOD(unknownNamesAndTypes)
	{
		ParameterList parameters(config(), defaults());
		IS_THROWN(ArgumentException, parameters.getInt("unknown"));
		IS_THROWN(ArgumentException, parameters.getFloat("unknown"));
		IS_THROWN(ArgumentException, parameters.getBool("unknown"));
		IS_THROWN(ArgumentException, parameters.getString("unknown"));
		IS_THROWN(ArgumentException, parameters.setInt("unknown", 1));
		IS_THROWN(ArgumentException, parameters.setFloat("unknown", 1.0));
		IS_THROWN(ArgumentException, parameters.setBool("unknown", true));
		IS_THROWN(ArgumentException, parameters.setString("unknown", "text"));
		IS_THROWN(ArgumentException, parameters.getFloat("height"));
		IS_THROWN(ArgumentException, parameters.getInt("scale"));
		IS_THROWN(ArgumentException, parameters.getString("visible"));
		IS_THROWN(ArgumentException, parameters.getBool("mode"));
		IS_THROWN(ArgumentException, parameters.setFloat("height", 1.0));
		IS_THROWN(ArgumentException, parameters.setInt("scale", 1));
		IS_THROWN(ArgumentException, parameters.setString("visible", "true"));
		IS_THROWN(ArgumentException, parameters.setBool("mode", true));
	}

	TEST_METHOD(stringList)
	{
		Parameter metadata("lists");
		metadata.append(ParameterDescription("modes", "Selected modes", ParameterType::STRINGLIST, {{ConstraintType::ALLOWED_VALUES, QByteArray("POINTS\tHEATMAP")}}));
		const QByteArrayList initial{"POINTS", "HEATMAP"};
		ParameterList parameters(metadata, {{"modes", QVariant::fromValue(initial)}});
		X_EQUAL(parameters.getStringList("modes"), initial);
		int changes = 0;
		QObject::connect(&parameters, &ParameterList::parameterChanged, [&changes]() { ++changes; });
		parameters.setStringList("modes", initial);
		I_EQUAL(changes, 0);
		const QByteArrayList reordered{"HEATMAP", "POINTS"};
		parameters.setStringList("modes", reordered);
		I_EQUAL(changes, 1);
		X_EQUAL(parameters.getStringList("modes"), reordered);
		IS_THROWN(ArgumentException, parameters.setStringList("modes", QByteArrayList({"POINTS", "INVALID"})));
		I_EQUAL(changes, 1);
		X_EQUAL(parameters.getStringList("modes"), reordered);
		parameters.setStringList("modes", {});
		IS_TRUE(parameters.getStringList("modes").isEmpty());
		I_EQUAL(changes, 2);
		parameters.setStringList("modes", {});
		I_EQUAL(changes, 2);
		const QByteArrayList duplicates{"POINTS", "POINTS"};
		parameters.setStringList("modes", duplicates);
		X_EQUAL(parameters.getStringList("modes"), duplicates);
		IS_THROWN(ArgumentException, parameters.getString("modes"));
		IS_THROWN(ArgumentException, parameters.setString("modes", "POINTS"));
		IS_THROWN(ArgumentException, parameters.getStringList("unknown"));
		IS_THROWN(ArgumentException, parameters.setStringList("unknown", initial));
	}

	TEST_METHOD(stringListDefaults)
	{
		Parameter metadata("lists");
		metadata.append(ParameterDescription("modes", {}, ParameterType::STRINGLIST, {{ConstraintType::ALLOWED_VALUES, QByteArray("POINTS\tHEATMAP")}}));
		const QList<QVariant> invalid_values{
			QVariant(), QByteArray("POINTS"), QString("POINTS"), 1,
			QStringList({"POINTS"}), QVariant::fromValue(QByteArrayList({"POINTS", "INVALID"}))
		};
		for (const QVariant& value : invalid_values)
		{
			IS_THROWN(ArgumentException, ParameterList(metadata, {{"modes", value}}));
		}
		ParameterList empty(metadata, {{"modes", QVariant::fromValue(QByteArrayList())}});
		IS_TRUE(empty.getStringList("modes").isEmpty());
		IS_THROWN(ArgumentException, ParameterDescription("modes", {}, ParameterType::STRINGLIST, {{ConstraintType::MIN, 1}}));
	}

	TEST_METHOD(unconstrainedStringList)
	{
		Parameter metadata("lists");
		metadata.append(ParameterDescription("entries", {}, ParameterType::STRINGLIST, {}));
		const QByteArrayList entries{"", "one\ttwo", "one", "one"};
		ParameterList parameters(metadata, {{"entries", QVariant::fromValue(entries)}});
		X_EQUAL(parameters.getStringList("entries"), entries);
		parameters.setStringList("entries", {"new"});
		X_EQUAL(parameters.getStringList("entries"), QByteArrayList({"new"}));
	}

	TEST_METHOD(nonEmptyString)
	{
		Parameter metadata("non_empty");
		metadata.append(ParameterDescription("text", {}, ParameterType::STRING, {{ConstraintType::NON_EMPTY, QVariant()}}));
		for (const QVariant& value : QList<QVariant>{QByteArray(), QByteArray(""), QString(), QString("")})
		{
			IS_THROWN(ArgumentException, ParameterList(metadata, {{"text", value}}));
		}
		ParameterList parameters(metadata, {{"text", QString("initial")}});
		int changes = 0;
		QObject::connect(&parameters, &ParameterList::parameterChanged, [&changes]() { ++changes; });
		IS_THROWN(ArgumentException, parameters.setString("text", ""));
		S_EQUAL(parameters.getString("text"), "initial");
		I_EQUAL(changes, 0);
		parameters.setString("text", " ");
		S_EQUAL(parameters.getString("text"), " ");
		I_EQUAL(changes, 1);
		for (ParameterType type : {ParameterType::INT, ParameterType::FLOAT, ParameterType::BOOL})
		{
			IS_THROWN(ArgumentException, ParameterDescription("number", {}, type, {{ConstraintType::NON_EMPTY, QVariant()}}));
		}
	}

	TEST_METHOD(nonEmptyStringList)
	{
		Parameter metadata("non_empty");
		metadata.append(ParameterDescription("entries", {}, ParameterType::STRINGLIST, {{ConstraintType::NON_EMPTY, QVariant()}}));
		const QByteArrayList initial{"one", "two"};
		IS_THROWN(ArgumentException, ParameterList(metadata, {{"entries", QVariant::fromValue(QByteArrayList({"one", ""}))}}));
		ParameterList parameters(metadata, {{"entries", QVariant::fromValue(initial)}});
		int changes = 0;
		QObject::connect(&parameters, &ParameterList::parameterChanged, [&changes]() { ++changes; });
		IS_THROWN(ArgumentException, parameters.setStringList("entries", QByteArrayList({"", "two"})));
		IS_THROWN(ArgumentException, parameters.setStringList("entries", QByteArrayList({QByteArray()})));
		X_EQUAL(parameters.getStringList("entries"), initial);
		I_EQUAL(changes, 0);
		parameters.setStringList("entries", {" ", "one"});
		I_EQUAL(changes, 1);
		//An empty list contains no empty strings.
		parameters.setStringList("entries", {});
		IS_TRUE(parameters.getStringList("entries").isEmpty());
		I_EQUAL(changes, 2);
		ParameterList empty(metadata, {{"entries", QVariant::fromValue(QByteArrayList())}});
		IS_TRUE(empty.getStringList("entries").isEmpty());
	}

	TEST_METHOD(nonEmptyAndAllowedValues)
	{
		Parameter metadata("non_empty");
		const QHash<ConstraintType, QVariant> constraints{{ConstraintType::NON_EMPTY, QVariant()}, {ConstraintType::ALLOWED_VALUES, QByteArray("\tA\tB")}};
		metadata.append(ParameterDescription("text", {}, ParameterType::STRING, constraints));
		metadata.append(ParameterDescription("entries", {}, ParameterType::STRINGLIST, constraints));
		ParameterList parameters(metadata, {{"text", QByteArray("A")}, {"entries", QVariant::fromValue(QByteArrayList({"A", "B"}))}});
		IS_THROWN(ArgumentException, parameters.setString("text", ""));
		IS_THROWN(ArgumentException, parameters.setString("text", "C"));
		IS_THROWN(ArgumentException, parameters.setStringList("entries", QByteArrayList({"A", ""})));
		IS_THROWN(ArgumentException, parameters.setStringList("entries", QByteArrayList({"A", "C"})));
		parameters.setString("text", "B");
		parameters.setStringList("entries", {"B"});
		S_EQUAL(parameters.getString("text"), "B");
		X_EQUAL(parameters.getStringList("entries"), QByteArrayList({"B"}));
	}

	TEST_METHOD(unconstrainedValues)
	{
		Parameter metadata("unconstrained");
		metadata.append(ParameterDescription("integer", {}, ParameterType::INT, {}));
		metadata.append(ParameterDescription("float", {}, ParameterType::FLOAT, {}));
		metadata.append(ParameterDescription("text", {}, ParameterType::STRING, {}));
		ParameterList parameters(metadata, {{"integer", std::numeric_limits<int>::min()}, {"float", 0.0}, {"text", QByteArray("")}});
		I_EQUAL(parameters.getInt("integer"), std::numeric_limits<int>::min());
		parameters.setInt("integer", std::numeric_limits<int>::max());
		I_EQUAL(parameters.getInt("integer"), std::numeric_limits<int>::max());
		parameters.setFloat("float", std::numeric_limits<double>::max());
		IS_TRUE(parameters.getFloat("float") == std::numeric_limits<double>::max());
		S_EQUAL(parameters.getString("text"), "");
		parameters.setString("text", QByteArray("one\ttwo"));
		S_EQUAL(parameters.getString("text"), "one\ttwo");
	}
};
