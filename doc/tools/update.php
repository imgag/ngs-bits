<?php

$dir = dirname(__FILE__);

function find_dll($dll)
{
	$output = [];
	exec("where {$dll}", $output, $exit_code);
	if ($exit_code!=0) return "";
	
	return trim($output[0]); //using the first is not 100% correct because where uses path only, but close enough
}

function find_dependencies($exe, $level=1)
{
	global $loc_cache;
	
	if ($level==1) print $exe."\n";
	
	$output = [];
	exec("C:\\Qt\\Tools\\mingw1310_64\\bin\\objdump.exe -p \"{$exe}\"", $output, $exit_code);
	foreach($output as $line)
	{
		if (strpos($line, "DLL Name:")!==false)
		{
			$dll = trim(explode(":", $line, 2)[1]);
			
			if (isset($loc_cache[$dll]))
			{
				print str_repeat("  ", $level)."{$dll} > ".$loc_cache[$dll]." (SEE ABOVE)\n";
			}
			else
			{
				$loc = find_dll($dll);
				if ($loc=="")
				{
					print str_repeat("  ", $level)."{$dll} > NOT FOUND!\n";
					trigger_error("{$dll} not found in PATH!", E_USER_ERROR);
				}
				else
				{
					print str_repeat("  ", $level)."{$dll} > {$loc}\n";
					$loc_cache[$dll] = $loc;
					if (substr($loc, 0, 20)!="C:\\Windows\\System32\\") //do not follow system DLLs...
					{
						find_dependencies($loc, $level+1);
					}
				}
			}
		}
	}
}

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

//add DLL folders to PATH under Windows
if (PHP_OS=="WINNT")
{	
	putenv("PATH={$dir}\\..\\..\\bin\\;{$dir}\\..\\..\\htslib\\lib\\;{$dir}\\..\\..\\libxml2\\libs\\;C:\\Qt\\6.8.3\\mingw_64\\bin\\;C:\\Qt\\Tools\\mingw1310_64\\bin\\;".getenv('PATH'));
	//debug missing DLLS
	//find_dependencies("{$dir}\\..\\..\\bin\\BamInfo.exe");
}

//get tool list
$tools_done = array("");
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