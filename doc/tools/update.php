<?php

$dir = dirname(__FILE__);

//get tools list
$tools = array();
$file = file("{$dir}/../../src/tools.pro");
foreach($file as $line)
{
	if (strpos($line, "SUBDIRS")!==FALSE)
	{
		if (strpos($line, "cpp")!==FALSE || strpos($line, "TEST")!==FALSE) continue;
		
		list(, , $tools[]) = explode(" ", trim($line));
	}
}

//copy DLLs to bin folder under Windows;
if (PHP_OS=="WINNT")
{
	$dlls = array(
				"{$dir}/../../htslib/lib/hts-3.dll",
				"{$dir}/../../htslib/lib/hts.dll.a",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5Core.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5XmlPatterns.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5PrintSupport.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5Network.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5Sql.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5Xml.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5Gui.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/Qt5Widgets.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/libgcc_s_dw2-1.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/libwinpthread-1.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/bin/libstdc++-6.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/plugins/platforms/qwindows.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/plugins/sqldrivers/qsqlmysql.dll",
				"C:/Qt/Qt5.9.5/5.9.5/mingw53_32/plugins/printsupport/windowsprintersupport.dll",
				);
				
	foreach($dlls as $source)
	{
		if (!file_exists($source))
		{
			trigger_error("Source file does not exist: {$source}", E_USER_ERROR);
		}
		
		$dest = "{$dir}/../../bin/".basename($source);
		if (!file_exists($dest))
		{
			print "copy $source\n";
			if (!copy($source, $dest))
			{
				trigger_error("Could not copy file: {$source}", E_USER_ERROR);
			}
		}
	}
}

//get tool list
$tools_done = array("");
foreach($tools as $tool)
{
	if (PHP_OS=="WINNT" && strpos($tool, "NGSDUpdateSvGenotype")!==FALSE) continue;
	
	//print tool name
	$tools_done[] = $tool;
	$exe = "{$dir}/../../bin/{$tool}";
	
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
	$out = "{$dir}/{$tool}.md";
	if(!is_file($out))
	{
		$save_to_disk = true;
	}
	else
	{
		$original = file($out, FILE_IGNORE_NEW_LINES);
		
		//compare file content
		$first_difference = true;
		for($i=0;$i<count($original);++$i)
		{
			//skip matching lines
			if($output[$i]==$original[$i]) continue;
			
			//skip verison lines
			if(preg_match("/\d[0-9_\.]+\d-\d{1,4}-g[a-z0-9]{7}/", $original[$i])!=0) continue;
			
			//print differences
			if ($first_difference)
			{
				print "Found differences for tool $tool:\n";
				$first_difference = false;
			}
			print "-\t'".$original[$i]."'\n";
			print "+\t'".$output[$i]."'\n";
			$save_to_disk = true;
		}
	}
	
	//store changes
	if($save_to_disk)
	{
		file_put_contents($out, implode("\n", $output));
		print "Updated documentation of $tool.\n";
	}
}
print "\n";

//remove all md files that are not necessary
$files = glob("{$dir}/*.md");
foreach($files as $file)
{
	$t = basename($file,".md");
	if(!in_array($t, $tools_done))
	{
		unlink($file);
		print "Removed documentation of $t.\n";
	}
}

?>