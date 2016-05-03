foreach ($num in 1,2,3,4) {
	& "C:\Users\Elias\Downloads\Feenix 2.4.3 client\Wow.exe" /run /SilentMode
}

Start-Sleep -s 8

$num=0
$i=0

Get-Process "Wow" | Foreach-Object {
	$num = ($i % 4) * 2
	$affinity = 3 -shl $num
	
	$_.ProcessorAffinity=$affinity
	$_.PriorityClass=[System.Diagnostics.ProcessPriorityClass]::RealTime
	$i += 1
}