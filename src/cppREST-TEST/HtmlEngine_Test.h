#ifndef HTMLENGINE_TEST_H
#define HTMLENGINE_TEST_H

#include "TestFramework.h"
#include "HtmlEngine.h"

TEST_CLASS(HtmlEngine_Test)
{
private:
	void test_convertIconNameToString()
	{
		QString icon = HtmlEngine::convertIconNameToString(FolderItemIcon::TO_PARENT_FOLDER);
		S_EQUAL(icon, "up_dir");

		icon = HtmlEngine::convertIconNameToString(FolderItemIcon::GENERIC_FILE);
		S_EQUAL(icon, "generic_file");

		icon = HtmlEngine::convertIconNameToString(FolderItemIcon::BINARY_FILE);
		S_EQUAL(icon, "binary_file");

		icon = HtmlEngine::convertIconNameToString(FolderItemIcon::CODE_FILE);
		S_EQUAL(icon, "code_file");

		icon = HtmlEngine::convertIconNameToString(FolderItemIcon::PICTURE_FILE);
		S_EQUAL(icon, "picture_file");

		icon = HtmlEngine::convertIconNameToString(FolderItemIcon::TEXT_FILE);
		S_EQUAL(icon, "text_file");

		icon = HtmlEngine::convertIconNameToString(FolderItemIcon::TABLE_FILE);
		S_EQUAL(icon, "table_file");

		icon = HtmlEngine::convertIconNameToString(FolderItemIcon::FOLDER);
		S_EQUAL(icon, "folder");
	}
};

#endif // HTMLENGINE_TEST_H
