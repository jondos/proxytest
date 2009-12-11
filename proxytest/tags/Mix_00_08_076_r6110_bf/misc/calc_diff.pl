#!/usr/bin/perl -w

use strict;

my $sum = 0;
my $negativePrepaidBytesSum = 0;
my $IN;
my $OUT;
if(@ARGV < 1)
{
	die "usage: ", $0, " logfile [outputfile]\n", 
		"If outputfile is not specified all messages will be written to stdout.\n";
}

open($IN, "<", "$ARGV[0]") or die "File ", $ARGV[0], " does not exist.\n";
if(@ARGV > 1)
{
	open ($OUT, ">", "$ARGV[1]") or die " Cannot open .", $ARGV[1],"\n";
}
else
{
	open  ($OUT, ">-") or die " Cannot write to Stdout.\n";
}

while(<$IN>)
{
	$sum += outputByteDiff($_);
	$negativePrepaidBytesSum += outputNegativePrepaidBytes($_);
}
close($IN);
if($sum > 0)
{
	print $OUT "Overall sum of confirmed bytes difference: ", $sum, " bytes.\n";
}
else
{
	print $OUT "No confirmed bytes difference found.\n";
}
if($negativePrepaidBytesSum > 0)
{
	print $OUT "Overall sum of all negative prepaid bytes: ", $negativePrepaidBytesSum, "\n";
}
close($OUT);

sub outputByteDiff
{
	my $inputLine = shift;
	my $delta = 0;
	if($inputLine =~ /\[(.*?),.*NOT been.*account nr ([0-9]*).*?([0-9]*)\/([0-9]*).*/)
	{
		my $logdate = $1;
		my $accountnumber = $2;
		my $confirmedBytes = $3;
		my $bytesToConfirm = $4;
		if($logdate && $accountnumber && $confirmedBytes && $bytesToConfirm)
		{
			$delta = $bytesToConfirm - $confirmedBytes;
			print $OUT "[", $logdate, "] Confirmed bytes difference for account ", $accountnumber, " is ", $delta, " bytes.\n";
		}
	}
	return $delta;
}

sub outputNegativePrepaidBytes
{
	my $inputLine = shift;
	my $negPrepaidBytes = 0;
	if($inputLine =~/\[(.*?),.*Stored.*?-([0-9]*) prepaid.*?nr\. ([0-9]*)/)
	{
		my $logdate = $1;
		$negPrepaidBytes = $2;
		my $accountnumber = $3;
		
		if($logdate && $accountnumber && $negPrepaidBytes)
		{
			print $OUT "[",$logdate, "] Found ", $negPrepaidBytes, " negative prepaid bytes for account ", $accountnumber, "\n";
		}
	}
	return $negPrepaidBytes;
	
}