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

//check URLs in MD files
$files = glob(dirname(__FILE__)."/*.md");
$files[] = dirname(__FILE__)."/../../README.md";
foreach($files as $file)
{
	//print tool name
	$md_text = file($file);
			
	//check for broken links
	foreach($md_text as $line)
	{
		preg_match_all('#\b(https|http|ftp)://[^,\s()<>]+(?:\([\w\d]+\)|([^,[:punct:]\s]|/))#', $line, $matches);
		if (count($matches[0])>0)
		{
			$url = $matches[0][0];
			if ($url=="https://github.com/imgag/ngs-bits") continue;
			
			print basename($file, ".md")." - {$url}\n";

			if(!url_exists($url))
			{
				print "\tMISSING!\n";
			}
		}
	}
}

//check relative markdown links
$files = [];
$files[] = dirname(__FILE__)."/../../README.md";
foreach($files as $file)
{
	$file = realpath($file);
	$base_dir = dirname($file);
	preg_match_all('/\[[^\]]*]\(([^)]+)\)/', file_get_contents($file), $matches);

	foreach ($matches[1] as $link)
	{
		//remove optional title
		$link = preg_split('/\s+"/', $link)[0];

		//skip URLs
		if (preg_match('/^(https?:|mailto:|#)/i', $link)) continue;

		//remove anchor part
		$link = explode('#', $link, 2)[0];
		if ($link=='') continue;
		
		$target = $base_dir."/".$link;
		if (!file_exists($target))
		{
			echo "BROKEN: {$link} in {$file}\n";
		}
	}
}

?>