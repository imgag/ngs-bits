<?php

$dir = dirname(__FILE__);

//get tools list
exec("grep SUBDIRS {$dir}/../../src/tools.pro | grep -v cpp | grep -v TEST | cut -f3 -d' '", $tools);

//get tool list
$tools_done = array();
foreach($tools as $tool)
{
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
			//skip verison lines
			if(preg_match("/\d\.\d-\d{3}-.{8}/", $original[$i]) != 0)
			{
				continue;
			}

			if($output[$i]!=$original[$i])
			{
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