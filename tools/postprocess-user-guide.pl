#!/usr/bin/env perl
use strict;
use warnings;

my $path = shift @ARGV or die "usage: $0 <markdown-file>\n";

open my $in, "<", $path or die "cannot read $path: $!\n";
local $/;
my $text = <$in>;
close $in;

$text =~ s/Microtonic User Guide_artifacts\//Microtonic%20User%20Guide_artifacts\//g;

sub trim {
	my ($s) = @_;
	$s =~ s/^\s+//;
	$s =~ s/\s+$//;
	return $s;
}

sub clean_toc {
	my ($block) = @_;
	my @items;

	for my $line (split /\n/, $block) {
		next unless $line =~ /^\|/;
		next if $line =~ /^\|[-\s|]+\|?$/;

		$line =~ s/^\|//;
		$line =~ s/\|$//;

		my @cols = map { trim($_) } split /\|/, $line;
		my $entry = trim(join " ", grep { length $_ } @cols);
		next unless length $entry;

		my $page = "";
		if ($entry =~ s/\.{3,}\s*(\d+)\s*$//) {
			$page = $1;
		}
		$entry = trim($entry);
		next unless length $entry;

		push @items, length($page) ? "- $entry, p. $page" : "- $entry";
	}

	return join("\n", @items) . "\n";
}

$text =~ s/(## Table of Contents\n\n)(.*?)(\n## I N T R O D U C T I O N)/$1 . clean_toc($2) . $3/se;

open my $out, ">", $path or die "cannot write $path: $!\n";
print {$out} $text;
close $out;
