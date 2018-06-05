<?php

function url_exists($url)
{
	$handle = curl_init($url);
	curl_setopt($handle, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($handle, CURLOPT_SSL_VERIFYPEER, false);
	curl_setopt($handle, CURLOPT_HEADER, true);
	curl_setopt($handle, CURLOPT_NOBODY, true);
	curl_setopt($handle, CURLOPT_USERAGENT, true);
	$headers = curl_exec($handle);
	curl_close($handle);
	
	if (empty($headers)) return false;
	
	$headers = explode(PHP_EOL, $headers);
	if (strpos($headers[0], "404") !== false)
	{
		return false;
	}

    return true;
}

$tools = glob(dirname(__FILE__)."/*.md");
foreach($tools as $tool)
{
	//print tool name
	$help = file($tool);
			
	//check for broken links
	foreach($help as $line)
	{
		preg_match_all('#\b(https|http|ftp)://[^,\s()<>]+(?:\([\w\d]+\)|([^,[:punct:]\s]|/))#', $line, $matches);
		if (count($matches[0])>0)
		{
			$url = $matches[0][0];
			if ($url=="https://github.com/imgag/ngs-bits") continue;
			
			print basename($tool, ".md")." - {$url}\n";

			if(!url_exists($url))
			{
				print "\tMISSING!\n";
			}
		}
	}
}

?>