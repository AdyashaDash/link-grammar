#!/usr/bin/perl -w
package TestLem;
use strict;
use BerkeleyDB;
use strict;
use locale;
use vars qw( %rule %tail %root %add %cat2 );

BEGIN {
        use Exporter   ();
        our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(&rules);
        %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
        @EXPORT_OK   = qw();
}
our @EXPORT_OK;
END { }
1;



sub get_data
{
	my ($s) = @_;
	my @res = ();
	for(my $i=0; $i <= length $s; $i++)
	{
		my ($left, $right) = (substr($s,0,$i), substr($s,$i));
		if(my $d = $root{$left}) {
			my @num = split /:/, $d;
			shift @num;
			foreach my $num(@num){
				if(my $c = $tail{"$num:$right"})
				{ 
					my @list = 
					unpack "A2" x (length($c)/2), $c;
					foreach my $l(@list) {
						my $rule = $rule{$l};
						if(my $t = $add{"$left:$num"})
						{
							$rule .= ",$t";
						}
						if($rule =~ /��-��/)
						{
							$rule =~ s/��-��/��/;
							push @res, $rule;
							$rule =~ s/��/��/;
							push @res, $rule;
						} else {
							push @res, $rule;
						}
					}
				}
			}
		}
	}
# �������� ������������ �������������
	for(my $i=0; $i <= $#res; $i++) {
		if($res[$i] =~ /�(.*��.*)��.*/ || $res[$i] =~ /�(.*��.*)��.*/) {
			my $rr = $1;
			my $l = 0;
			my $n = 0;
			foreach (@res) {
				$l = 1 if /�$rr��.*/;
				$n = 1 if /�$rr��.*/;
			}
			$res[$i] .= ",��" if $l;
			$res[$i] .= ",��" if $n;
		}
	}
	my %res = ();
	foreach (@res) {$res{$_} = 1;}
	return keys %res;
}

sub parse
{
	my ($s) = @_;
	my $res = "";
	$res .= "n" if $s =~ /^�(,|$)/;
	$res .= "a" if $s =~ /^�(,|$)/;
	$res .= "v" if $s =~ /^�(,|$)/;
	$res .= "e" if $s =~ /^�(,|$)/;
	$res .= "p" if $s =~ /^����(,|$)/;
	$res .= "c" if $s =~ /^����(,|$)/;
	$res .= "k" if $s =~ /^����-�(,|$)/;
	$res .= "i" if $s =~ /^����(,|$)/;
	$res .= "j" if $s =~ /^�����(,|$)/;
	$res .= "o" if $s =~ /^����(,|$)/;
	$res .= "q" if $s =~ /^����(,|$)/;
	$res .= "x" if $s =~ /^�����(,|$)/;
	$res .= "y" if $s =~ /^�����(,|$)/;
	$res .= "f" if $s =~ /^����(,|$)/;
	$res .= "m" if $s =~ /^��(,|$)/;
	$res .= "w" if $s =~ /^��-�(,|$)/;
	$res .= "u" if $s =~ /^��-�����(,|$)/;
	$res .= "n" if $s =~ /,��(,|$)/;
	$res .= "s" if $s =~ /,��(,|$)/;
	$res .= "n" if $s =~ /,��(,|$)/;
	$res .= "p" if $s =~ /,��(,|$)/;
	$res .= "p" if $s =~ /,���(,|$)/;
	$res .= "d" if $s =~ /,���(,|$)/;
	$res .= "s" if $s =~ /,���(,|$)/;
	$res .= "d" if $s =~ /,���(,|$)/;
	$res .= "i" if $s =~ /,���(,|$)/;
	$res .= "n" if $s =~ /,���(,|$)/;
	$res .= "f" if $s =~ /,���(,|$)/;
	$res .= "p" if $s =~ /,���(,|$)/;
	$res .= "v" if $s =~ /,���(,|$)/;
	$res .= "1" if $s =~ /,1�(,|$)/;
	$res .= "2" if $s =~ /,2�(,|$)/;
	$res .= "3" if $s =~ /,3�(,|$)/;
	
	$res .= "a" if $s =~ /,����(,|$)/;
	$res .= "d" if $s =~ /,��(,|$)/;
	$res .= "l" if $s =~ /,��(,|$)/;
	$res .= "m" if $s =~ /,��(,|$)/;
	$res .= "f" if $s =~ /,��(,|$)/;
	$res .= "n" if $s =~ /,��(,|$)/;
	$res .= "s" if $s =~ /,��(,|$)/;
	$res .= "p" if $s =~ /,��(,|$)/;
	$res .= "i" if $s =~ /,��(,|$)/;
	$res .= "g" if $s =~ /,��(,|$)/;
	$res .= "d" if $s =~ /,��(,|$)/;
	$res .= "t" if $s =~ /,��(,|$)/;
	$res .= "v" if $s =~ /,��(,|$)/;
	$res .= "p" if $s =~ /,��(,|$)/;
	$res .= "s" if $s =~ /,��(,|$)/;
	$res .= "s" if $s =~ /,�����(,|$)/;
	$res .= "f" if $s =~ /,���(,|$)/;
	$res .= "n" if $s =~ /,���(,|$)/;
	return $res;
}


sub TestLem {
	(my $lexer) = @_;


#my %cat2 = ();

open FILE, "base.dict";
while (<FILE>) {
	next unless /^\%/;
	next unless /morph/;
	my $str = $_;
	chomp $str;
#% morph{"�����,���"} ="<������-��-��-sub> &  (<������-pr> or <�����-������-pr>)";
	(my $nr, my $rl) = split(/\=/, $str);
	$nr =~ s/^\%\ +morph\{\"//; $nr =~ s/\"\}\ *$//;
	$rl =~ s/^\ ?\"//; $rl =~ s/\"\;\ ?$//;
	$cat2{$nr} = $rl;
}
close FILE;



tie %rule, "BerkeleyDB::Hash",
	-Filename => "rule.db" or die "Error blin ";
tie %tail, "BerkeleyDB::Hash",
	-Filename => "tail.db" or die "Error blin ";
tie %root, "BerkeleyDB::Hash",
	-Filename => "root.db" or die "Error blin ";
tie %add, "BerkeleyDB::Hash",
	-Filename => "add.db" or die "Error blin ";


my %words = ();

my @rows = split(/\n/, $lexer);

foreach my $s1 (@rows) {
#	my $s = lc $s1;
	my $s = $s1;
	chomp $s;
	$words{$s} = 1;
}

my @nonwords = qw(�� ��� ��� � ���� ����� ��� ��� ��� ��� ��� ������);

foreach (@nonwords) {
	delete $words{$_};
}

my $out = "";

foreach my $s(sort keys %words) {
#	my @r = get_data(lc $s);
	my @r = get_data($s);
	foreach my $i(@r) {
		if (rules($i)) {
			$out.="$s." . parse($i) . ": " . rules($i) . "; % $i\n";
		} else {
			system(" echo \"rules $i not defined $s\" >> /tmp/testlem.log");
		}
	}
}

untie %rule;
untie %tail;
untie %root;
untie %add;

return $out;

}


sub get_tokens {
	my $str = shift;
	my %hash = ();
	        $str =~ s/>([^<]*?)</></g;
                $str =~ s/>([^>]*)$//;
                $str =~ s/^(.*?)<//;
	my @tokens = split(/></,$str);
	foreach (@tokens) {
		$hash{$_}++;
	}
	my @tokens2 = keys %hash;
	return @tokens2;
}

sub rules($) {
        my ($cur) = @_;
        $_ = $cur;
        chomp;
	s/���,//;
	s/���,//;
        return $cat2{$_};
}
