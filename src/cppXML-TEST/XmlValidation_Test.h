#include "TestFramework.h"
#include "XmlHelper.h"


TEST_CLASS(XmlValidation_Test)
{
Q_OBJECT
private slots:
    void test_xml_validation()
    {
        QString xml_error = XmlHelper::isValidXml(":data/germline_report2.xml");
        if(xml_error!= "")
        {
            THROW(ProgrammingException, "Invalid XML file:\n" + xml_error);
        }
        IS_TRUE(xml_error.isEmpty());

        xml_error = XmlHelper::isValidXml(":data/tumor_only_report_broken_xml.xml");
        IS_FALSE(xml_error.isEmpty());
    }

    void test_html_as_xml_validation()
    {
        QString html_as_xml_error = XmlHelper::isValidXml(":data/germline_sheet1.html");
        if(html_as_xml_error!= "")
        {
            THROW(ProgrammingException, "Invalid HTML file:\n" + html_as_xml_error);
        }
        IS_TRUE(html_as_xml_error.isEmpty());

        html_as_xml_error = XmlHelper::isValidXml(":data/germline_sheet1_broken.html");
        IS_FALSE(html_as_xml_error.isEmpty());
    }

    void test_xml_validation_against_schema()
	{
        QString xml_error = XmlHelper::isValidXml(TESTDATA("data/somatic_report.xml"), TESTDATA("data/SomaticReport.xsd"));
        if(xml_error!= "")
        {
            THROW(ProgrammingException, "Invalid somatic report XML file:\n" + xml_error);
        }
        IS_TRUE(xml_error.isEmpty());

        xml_error = XmlHelper::isValidXml(TESTDATA("data/germline_report1.xml"), TESTDATA("data/GermlineReport.xsd"));
        if(xml_error!= "")
        {
            THROW(ProgrammingException, "Invalid germline report XML file:\n" + xml_error);
        }
        IS_TRUE(xml_error.isEmpty());

        xml_error = XmlHelper::isValidXml(TESTDATA("data/tumor_only_report_broken_schema.xml"), TESTDATA("/data/TumorOnlyNGSReport_v1.xsd"));
        IS_FALSE(xml_error.isEmpty());
	}
};
