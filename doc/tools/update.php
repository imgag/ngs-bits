<?php

function contains($haystack, $needle)
{
	return strpos($haystack,$needle)!== false;
}

//remove .md files in this folder
$files = glob("./*.md");
foreach($files as $file)
{
	unlink($file);
}


//copy DLLs to bin path
$dlls = array("Qt5Core.dll", "libgcc_s_dw2-1.dll", "libwinpthread-1.dll", "libstdc++-6.dll", "Qt5XmlPatterns.dll", "Qt5Network.dll", "Qt5Sql.dll", "Qt5Xml.dll", "Qt5Gui.dll", "Qt5Widgets.dll");
$qt_path = "C:\\Qt\\Qt5.5.0\\5.5\\mingw492_32\\bin\\";
$bin_path = "..\\..\\bin\\";
copy("..\\..\\bamtools\\lib\\libbamtools.dll", $bin_path."libbamtools.dll");
foreach($dlls as $dll)
{
	copy($qt_path.$dll, $bin_path.$dll);
}

//get tool list
$files = glob($bin_path."*.exe");
foreach($files as $exe)
{
	//skip tests
	if (contains($exe, "TEST")) continue;

	//skip GUI tools
	if (contains($exe, "GSvar") || contains($exe, "GSmix") || contains($exe, "GSpheno") ) continue;
		
	//print tool name
	$tool = basename($exe, ".exe");
	print $tool."\n";
	
	//get help text
	$help = array();
	exec("$exe --help 2>&1", $help, $ret);
	if ($ret!=0) trigger_error("Executing tool $exe failed: ".implode("\n", $help), E_USER_ERROR);
	
	//store help text
	$output = array();
	$output[] = "### $tool tool help";
	foreach($help as $line)
	{
		$output[] = "\t".$line;
	}
	$output[] = "[back to ngs-bits](https://github.com/marc-sturm/ngs-bits)";
	file_put_contents($tool.".md", implode("\n", $output));
}


?>