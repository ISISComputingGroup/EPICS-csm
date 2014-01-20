eval 'exec perl -S $0 ${1+"$@"}' # -*- Mode: perl -*-
    if 0;                         
# the above is a more portable way to find perl
# ! /usr/bin/perl

#  This software is copyrighted by the
#  Helmholtz-Zentrum Berlin fuer Materialien und Energie GmbH (HZB),
#  Berlin, Germany.
#  The following terms apply to all files associated with the software.
#  
#  HZB hereby grants permission to use, copy and modify this
#  software and its documentation for non-commercial, educational or
#  research purposes provided that existing copyright notices are
#  retained in all copies.
#  
#  The receiver of the software provides HZB with all enhancements, 
#  including complete translations, made by the receiver.
#  
#  IN NO EVENT SHALL HZB BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
#  SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE
#  OF THIS SOFTWARE, ITS DOCUMENTATION OR ANY DERIVATIVES THEREOF, EVEN 
#  IF HZB HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  
#  HZB SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED
#  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
#  PURPOSE, AND NON-INFRINGEMENT. THIS SOFTWARE IS PROVIDED ON AN "AS IS"
#  BASIS, AND HZB HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
#  UPDATES, ENHANCEMENTS OR MODIFICATIONS.


#pragmas:
use strict;
#activate perl-extensions:
#use lib "$ENV{HOME}/pmodules";
#use perl_site;

use FindBin;
use Getopt::Long;

BEGIN
  { # search the arguments for the "--locallibs"
    # option. If it is found, remove the option
    # and add $FindBin::Bin to the head of the
    # module search-path.
    if (exists $ENV{MYPERLLIBS})
      { my @dirs=split(/:/,$ENV{MYPERLLIBS});
        unshift @INC,split(/:/,$ENV{MYPERLLIBS});
      };
  };

use vars qw($opt_help $opt_quiet $opt_multiple $opt_header $opt_part $opt_check);

my $version = "1.6p";


# debugging:
my $dump_input =0; 
my $dump_output=0; 

my %wanted_parts;

if (($#ARGV==0) && ($ARGV[0] eq '-h'))
  { print_help();
    exit;
  };  

if (!GetOptions("help","quiet|q", "multiple|m","header|h=s","part|p=s","check|c"))
  { die "parameter error, use \"$0 -h\" to display the online-help\n"; };

if ($opt_help)
  { print_help();
    exit;
  };  

if ($#ARGV<0)
  { print_help();
    exit;
  };  

if (defined $opt_part)
  { $wanted_parts{$opt_part} = 1; 
    while ($ARGV[0]=~ /^\s*\d\s*$/) # an integer
      { my $part= shift(@ARGV);
        $wanted_parts{$part}= 1;
      };
  };

if (defined $opt_multiple)
  { if (!defined($opt_header))
      { die "error, -h option missing\n"; };
  };

my @files= $ARGV[0];

if (!defined($opt_header))
  { $opt_header= $files[0];
    if ($opt_header=~ /\.\w+$/)
      { $opt_header=~ s/\.\w+$/\.h/; }
    else
      { $opt_header.= ".h"; };
  };       

my $org_header;

if ((defined $opt_check) && (-e $opt_header) && (!$dump_output))
  { $org_header= $opt_header;
    $opt_header= "HGEN$$";
  };

if (defined($opt_multiple))
  { if ($#ARGV>=0) 
      { push @files,@ARGV; };
  };

if ($dump_output)
  { *OUT= *STDOUT; }
else
  { open(OUT, ">$opt_header") || die "unable to open $opt_header\n"; };

foreach my $file (@files)
  { local *IN;
    open(IN, $file) || die "unable to open $file\n";
    if (!$opt_quiet)
      { print "header file generated: $opt_header\n";
        print "processing $file...\n";
      };
    process_file(*IN,*OUT);
    close(IN);
  }  

if (!$dump_output)
  { close(OUT); };

if (defined $org_header)
  { 
    my $s1= `cksum $org_header`;
    my $s2= `cksum $opt_header`;
    my($n1)= ($s1=~ /^(\d+)/);
    my($n2)= ($s2=~ /^(\d+)/);

    if ($n1 == $n2) # header-file was unchanged
      { 
        unlink($opt_header) || die "unable to remove $opt_header\n"; 
      }
    else
      { 
        unlink($org_header) || die "unable to remove $opt_header\n";
        rename($opt_header,$org_header) ||
	  die "unable to rename $opt_header to $org_header\n";
        if ($opt_quiet)
          { print "header generated: $opt_header\n"; };
          };
  }
else
  { if ($opt_quiet)
      { print "header generated: $opt_header\n"; };
  }


sub process_file
  { my($in,$out)= @_; # typeglobs !!!

    my $in_string =0; # 1: within a double-quoted string
    my $in_comment=0; # -1: a one-line comment, 1: a real comment
    my $cmt_start=0;

    my $emit_flag=-1; # no of lines to emit, 0: none, -1: emit all
                      # >0: decremented each line
    my $skip_emit=0;  # 1: skip emission of THIS line
    my $post= undef;  # used to add ';' for extern statements
    my $postre= undef;# used to add ';' for extern statements, NOT YET USED !!!
    my $pre = undef;  # used to add ';' for extern statements
    my $indent=0;     # indent-numbers for all lines except $pre - lines
    my $uncomment=0;  # remove '/*' and '*/' from the comment

    my $active=1;    # state of the part (@PS() and @PE())

    my %found_parts;

    my $old_emit_flag;

    while(my $line=<$in>)
      { 
	chomp($line);
	if ($emit_flag>0)
	  { $emit_flag--; 
	    if (($emit_flag==0) && (defined $old_emit_flag))
	      { $emit_flag= $old_emit_flag;
	        $old_emit_flag= undef;
	      };	
	  };
	$skip_emit=0; 

	$cmt_start= -1;
	$in_string= 0;

	print "IN----------|$line|\n" if ($dump_input);
	for(;;)
	  { 
            # scan for comment-starts:
	    if ($in_string)
	      { 
	        if ($line=~ /\G.*?(\\\"|\")/g)
	          { my $match= $1;
		    if   ($match eq '\"')   # quoted double-quote
	              { next; }
	            elsif ($match eq '"')   # double-quote
		      { $in_string=0; next; }
		    else
		      { die "internal error"; };
		  }
		else
		  { last; }; # leave for-loop
	      };
	    if ($in_comment==0)
              { 
		if ($line=~ /\G.*?(\\"|"|\/\/|\/\*)/g)
	          { my $match= $1;
		    if   ($match eq '\"')   # quoted double-quote
	              { next; }
		    elsif ($match eq '"')   # double-quote
		      { 
		        $in_string=1; next; 
		      }
		    elsif ($match eq '/*')  # c-comment
        	      { $in_comment= 1;
	        	$cmt_start= pos($line)-2; 
			next;
		      }
		    elsif ($match eq '//')
		      { $in_comment= -1;
	        	$cmt_start= pos($line)-2;
			next;
		      }
		    else
		      { die "internal error"; };
		  }       
		else 
		  { last; }; # leave for-loop
	      }
	    else
	      { 
		# within comments, scan for commands in the form "@CCC" where C is
		# an upper-case char
                while ($line=~ /\G.*?(\*\/|\@(?:U|IL|EL|ITI|IT|ETI|ET|
		                             EM|EXI|EX|PS|PE))/gx)
		  { 
		    my $cmd = $1;
		    my $epos= pos($line)-1;
	            my $mpos= pos($line)-length($cmd); # save match-position,
		    if    ($cmd eq '*/') # comment-end found
		      { if ($uncomment)
	        	  { $old_emit_flag= undef;
			    $emit_flag= 1; $skip_emit=0;
			    my $pos= pos($line)-2;
			    substr($line,$pos,2)= "";
			    $uncomment=0; 
                	  };
	        	$in_comment= 0;
			last; # leave while-loop
        	      }

		    elsif ($cmd eq '@PS')
		      { if (!($line=~ /\G\((.*?)\)/gc))
		          { die '@PS' . ": args missing\n"; };
			my $arg= $1;
			$epos= pos($line)-1; # save match-position
			$found_parts{$arg}=1; 
			$active= check_part(\%wanted_parts,\%found_parts);
		      }
		    elsif ($cmd eq '@PE')
		      { if (!($line=~ /\G\((.*?)\)/gc))
		          { die '@PE' . ": args missing\n"; };
			my $arg= $1;
			$epos= pos($line)-1; # save match-position
			$found_parts{$arg}=0; 
			$active= check_part(\%wanted_parts,\%found_parts);
		      }
		    elsif (!$active) # do not eval commands when not active
		      { next; }
		    elsif ($cmd eq '@U')
		      { substr($line,$mpos,$epos-$mpos+1)= "";
			substr($line,$cmt_start,2)= "";
			pos($line)= $cmt_start;
			$uncomment=1;
			$old_emit_flag= undef;
			$emit_flag= -1; $skip_emit=0; 
			next;
		      }
		    elsif ($cmd eq '@IL')
		      { $old_emit_flag= $emit_flag; $emit_flag= 1; }
		    elsif ($cmd eq '@EL')
		      { $skip_emit= 1; }
		    elsif ($cmd eq '@IT')
		      { $old_emit_flag= undef; $emit_flag= -1; $skip_emit=1; }
		    elsif ($cmd eq '@ITI')
		      { $old_emit_flag= undef; $emit_flag= -1; $skip_emit=0; }
		    elsif ($cmd eq '@ET')
		      { $old_emit_flag= undef; $emit_flag= 1; }
		    elsif ($cmd eq '@ETI')
		      { $old_emit_flag= undef; $emit_flag= 0; }
		    elsif ($cmd eq '@EM')
		      { 
		        if (!($line=~ /\G\(\"(.*?)\"\)/gc))
		          { die '@EM' . ": args missing\n"; };
		        my $arg= $1;
			# direct like my $arg= ($line=~....) doesn't set
			# pos($line) correctly !!
			$epos= pos($line)-1; # save match-position
			emit(conv_text($arg),$active,$out);
		      }
		    elsif ($cmd=~ /^\@EX/)
		      { my $immediate= ($cmd eq '@EXI');
		        my $arg= 1;

			# support the old and the 
			# new style like '@EX' '@EX1' or '@EX(1)':
                        if    ($line=~ /\G(\d+)/gc)
			  { $arg= $1;
			    $epos= pos($line)-1; # save match-position
			  }
			elsif ($line=~ /\G\((.*?)\)/gc) 
			  { $arg= $1; 
			    $epos= pos($line)-1; # save match-position
			  };
			$old_emit_flag= $emit_flag;
			if ($immediate)
			  { $emit_flag= $arg; $skip_emit=0; }
			else
                          { $emit_flag= $arg + 1; $skip_emit=1; };
			$pre= "extern ";
			$indent= length($pre);
			$post= ';';
			$postre= 's/=[^\)]+$//';
		      }
		    else
		      { print STDERR "unknown command: $cmd\n"; };

		    # now remove the command from the string:  
		    my $ch;
		    if ($epos>= length($line)-1)
		      { $ch= undef; }
		    else
		      { $ch= substr($line,$epos+1,1);
			if (($ch eq '*') || ($ch eq '@')) 
			  { $ch= undef; };
		      };

		    if ($ch)
		      { substr($line,$mpos,$epos-$mpos+1)= 
		                 $ch x ($epos-$mpos+1);
		      }
		    else
		      { substr($line,$mpos,$epos-$mpos+1)= ""; };
		    pos($line)= $mpos;
		  }; # while ($line=~ /.../)
		last; # no further commands and no comment-end
	      } # if ($in_comment==0) ... else ...
	  }; # for(;;)

	# emit the line, if necessary      
	if ((!$skip_emit) && ($emit_flag!=0))
	  { pos($line)=0;
	    $line=~ s/\/\*\s*?\*\///g;
	    $line=~ s/\/\/\s*$//;
            $line=~ s/\s+$//;
	    if (($indent>0) && (!$pre))
	      { $line= (" " x $indent) . $line; };
            if (($line) && ($pre))
	      { $line= $pre . $line; 
		$pre= undef;
	      };
	    if (($emit_flag==1) && ($post) && ($line)) # add_char && last line 
	      { # eval('$line=~ ' . $postre) if ($postre);
	        # the following is less generic but faster:
	        $line=~ s/=[^\)]+$//;
		$postre= undef;
	        $line.= $post;
		$post= undef;
	      };
	    if ($emit_flag==1)
	      { $indent= 0; }; # remove indent 
	    $line.= "\n";
	    emit($line,$active,$out); # if ($line); 
	  };

	# remove comment-status for C++ comments  
	if ($in_comment==-1)
	  { $in_comment=0; };

      };
  }

sub check_part
  { my($r_wanted,$r_current)= @_;

    return (1) if (!%$r_wanted);

    foreach my $key (keys %$r_current)
      { next if (!$r_current->{$key});
        if ($r_wanted->{$key})
	   { return(1); };
      };
    return(0);
  }  

sub emit
  { my($text,$active,$fh)= @_; # $fh: a typeglob 

    return if (!$active); # the global "active" variable
    if ($dump_output)
      { $text=~ s/\n$//;
        print $fh "OUT---------|$text|\n"; }
    else
      { print $fh $text; };
  }

sub conv_text
  { my($text)= @_;

    $text=~ s/\\\"/\"/g;
    $text=~ s/\/\@/\/\*/g;
    $text=~ s/\@\//\*\//g;
    $text=~ s/\\n/\n/gm;
    return($text);
  }  

sub print_help
  { 
    print <<END
************* $FindBin::Script $version *****************
The perl-based header-generator program

usage: $FindBin::Script {options} [filename(s)]
options:
  --help : this help
  -m --multiple: more than one input-file given
      all files will be processed, -h option required !!
  -h --header {header-file}:
      specify the name of the header-file to create

  -p [number {number ...}]
      specify the parts of the source-file that are to be 
      processed, a source-file may have several parts (0..19)
      that may be intermixed. See also the \@PE()-command
  -c check wether the new header-file differs from the old one. If no
     difference is found, the header file is left intact. Useful when hgen.p
     is called from within a makefile.
  -q less messages on the screen

hgen-commands in sourcecode:
a command starts with a \'\@\' and is followed by 2 to 3 letters. A command
must always be splaced within a c-comment.
Example for a command: /*\@EX(1)*/ is the command 'EX(1)'

List of commands:
 EL  exclude line
 IL  include line
 ET  exclude text (until IT),
     U, IL, EX, EXI override this for their scope
 IT  include text (was excluded by ET)
 EX  generate extern-statement in next line
 EX(1)...EX(n)
     generate extern-statement and place finishing ';' 
     0 bis n lines below
 ET, IT or EX followed by 'I' cause the command to take
     effect in the current line instead of the next line
 U   uncomment whole comment (that may span several lines)
     this command should not be followed immediately by 
     alphanumeric chars for future compability
 EM("text")
     emit text (may contain '\\n' and '\\"')
     \'/\@\' in the text is changed to '/*', '@/' is changed 
      to '*/'
 PS(number)
 PE(number)
     PS: define the start of a <part> in the source-file. 
     PE: define the end of a <part> in the source-file. This
     works together with the -p command-line option of hgen.
     PS and PE define "parts" in the source file. Only the parts, that
     were specified with the "-p" command are evaluated.
END
  }

