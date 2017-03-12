#!/usr/local/bin/perl
#
# File:     sample_filter.pl
# Created:  13-Feb-1998

# $Header: /opt/nr/rcs/cmd/typhoond/sample_filter.pl,v 1.3 1998/02/16 07:19:42 rmf Exp $ 

# COPYRIGHT 1998
# HighWind Software, Inc.
# All Rights Reserved.
# 
# P.O. Box 451
# Marlborough, MA 01752
# 508-303-6875

#
# Simple illustration of a "MAKE MONEY FAST" Typhoon/Breeze filter
#

$| = 1; # Flush STDOUT
$good_article = "335\r\n";
$bad_article = "435\r\n";

# Initialize
$result = $good_article;

# Loop on input
while(defined($line = <>)) {
    # If the Subject contains "MAKE MONEY FAST", reject the article.
    if ($line =~ /^Subject:/) {
	if ($line =~ /MAKE MONEY FAST/) {
	    $result = $bad_article;
	} else {
	    $result = $good_article;
	}
    }

    if ($line eq ".\r\n") { print $result; }
}
