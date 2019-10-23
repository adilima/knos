
#############################################################################################
# This will calculate the correct index into Page Directory for 2 MB pages.
# Current implementation should work (as long as the given address in in 2 MB page boundary.
#############################################################################################

print "Virtual Address (in hex): ";

$strTest = <STDIN>;
$vaddr = hex $strTest;
print "Bytes => " .$vaddr ."\n";

$last_bits = 0xffff;
$start_p4  = ($vaddr >> 39) & 0x1ff;
$start_pdp = ($vaddr >> 30) & 0x1ff;

$start_vaddr = ($last_bits << 48) | ($start_p4 << 39) | ($start_pdp << 30);

printf("Start virtual address for this group => 0x%X\n", $start_vaddr);

$distance = $vaddr - $start_vaddr;
printf("Distance from start vaddr => %llu bytes\n", $distance);

$page_directory_index = $distance / 0x200000;
printf("Page Directory index for 2 MB pages should be: %llu\n", $page_directory_index);
printf("At PML4 Table index %llu\nPDP index %llu\nFrame 0x%X\n", 
	($vaddr >> 39) & 0x1ff,
	($vaddr >> 30) & 0x1ff,
	$vaddr & 0xfff);

