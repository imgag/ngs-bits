<?php

$dir = dirname(__FILE__)."//";

function contains($haystack, $needle)
{
	return strpos($haystack,$needle)!== false;
}

//copy DLLs to bin path
$dlls = array("Qt5Core.dll", "libgcc_s_dw2-1.dll", "libwinpthread-1.dll", "libstdc++-6.dll", "Qt5XmlPatterns.dll", "Qt5Network.dll", "Qt5Sql.dll", "Qt5Xml.dll", "Qt5Gui.dll", "Qt5Widgets.dll");
$qt_path = "C:\\Qt\\Qt5.5.0\\5.5\\mingw492_32\\bin\\";
if(!is_dir($qt_path))	$qt_path = "C:\\Qt\\Qt5.6.0\\5.6\\mingw49_32\\bin\\";
$bin_path = $dir."..\\..\\bin\\";
copy($dir."..\\..\\bamtools\\lib\\libbamtools.dll", $bin_path."libbamtools.dll");
foreach($dlls as $dll)
{
	copy($qt_path.$dll, $bin_path.$dll);
}

//get tool list
$tools = array();
$files = glob($bin_path."*.exe");
foreach($files as $exe)
{
	//skip tests
	if (contains($exe, "TEST")) continue;

	//skip GUI tools
	if (contains($exe, "GSvar") || contains($exe, "GSmix") ) continue;
		
	//print tool name
	$tool = basename($exe, ".exe");
	$tools[] = $tool;
	$output = array();
	
	//get help text
	$help = array();
	exec("$exe --help 2>&1", $help, $ret);
	if ($ret!=0) trigger_error("Executing tool $exe failed: ".implode("\n", $help), E_USER_ERROR);
	//store help text
	$output[] = "### $tool tool help";
	foreach($help as $line)
	{
		$output[] = "\t".$line;
	}
	
	//get changelog text and prepare output
	$change	= array();
	exec("$exe --changelog 2>&1", $change, $ret);
	if ($ret!=0) trigger_error("Executing tool $exe failed: ".implode("\n", $change), E_USER_ERROR);
	//store change test
	$output[] = "### $tool changelog";
	for($i=0; $i<count($change);++$i)
	{
		$output[] = "\t".$change[$i];
	}
	$output[] = "[back to ngs-bits](https://github.com/imgag/ngs-bits)";
		
	//check if is available and compare to current output
	$save_to_disk = false;
	$out = $dir.$tool.".md";
	if(!is_file($out))
	{
		$save_to_disk = true;
	}
	else
	{
		$original = file($out,FILE_IGNORE_NEW_LINES);
		
		//compare file content
		for($i=0;$i<count($original);++$i)
		{
			$skip_line = false;
			if(preg_match("/\d\.\d-\d{3}-.{8}/", $original[$i]) != 0)
			{
				$skip_line = true;
			}

			if($output[$i]!=$original[$i] && !$skip_line)
			{
				print "Found differences for tool $tool:\n";
				print "  old:\t'".$original[$i]."'\n";
				print "  new:\t'".$output[$i]."'\n";
				$save_to_disk = true;
			}
		}
	}
	
	//store changelog
	if($save_to_disk)
	{
		file_put_contents($dir.$tool.".md", implode("\n", $output));
		print "Writing new md-file for tool $tool.\n";
	}
	else
	{
		print "Nothing to do for tool $tool.\n";
	}
}

//remove all md files that are not necessary
$files = glob("*.md");
foreach($files as $file)
{
	$t = basename($file,".md");
	if(!in_array($t,$tools))
	{
		unlink($file);
		print "Removed $file (no tool $t found).\n";
	}
}

?>