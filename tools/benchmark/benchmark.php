<?php

include("/mnt/users/bioinf/megSAP/src/Common/all.php");


function add_times(&$data, $filename)
{
	$file = file($filename);
	foreach($file as $line)
	{
		$line = trim($line);
		$parts = explode("\t", $line);
		if (count($parts)<3) continue;
		list($status, $name, $time) = $parts;
		$data[$name][] = strtr($time, array("s"=>""));
	}
}

//load current benchmarks
$times_curr = [];
for ($i=1; $i<$argc; ++$i)
{
	add_times($times_curr, $argv[$i]);
}

//load nightly test files of last 3 days (i.e. latest 6 files)
$times_nightly = [];
list($test_files) = exec2("find /mnt/users/all/http/NightlyTests/tests/ -name \"cpp_t*.*\" | sort | tail -n6");
foreach($test_files as $file)
{
	add_times($times_nightly, $file);
}

//extract test names
$test_names = array_unique(array_merge(array_keys($times_curr), array_keys($times_nightly)));
sort($test_names);

//print result
print "#test\tsec_nightly\tsec_current\trel_change\n";
foreach($test_names as $name)
{
	//caclculate average time nightly
	$sec_nightly = "-";
	if (isset($times_nightly[$name]))
	{
		$values = $times_nightly[$name];
		$sec_nightly = array_sum($values)/count($values);
		$sec_nightly = number_format($sec_nightly, 4);
	}
	
	//caclculate average time current
	$sec_curr = "-";
	if (isset($times_curr[$name]))
	{
		$values = $times_curr[$name];
		$sec_curr = array_sum($values)/count($values);
		$sec_curr = number_format($sec_curr, 4);
	}
	
	//calcualte change
	$change = "-";
	if ($sec_nightly!="-" && $sec_curr!="-" && $sec_nightly>=0.01)
	{
		$change = number_format(100*$sec_curr/$sec_nightly, 2);
	}
	
	print "{$name}\t{$sec_nightly}\t{$sec_curr}\t{$change}\n";
}
?>
