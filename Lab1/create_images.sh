#!/bin/csh
# CSci4061 Spring 2017 Assignment 1
# Name: Christopher Bradshaw, Christopher Dang
# Student ID: 5300734, 5323765
# CSELabs machine: <machine> 
# Additional comments

# Must have at least one argument (pattern)
if ($#argv < 3) then
	printf "USAGE: create_images.sh input_directory output_directory pattern_1 [pattern_2] ... [pattern_n]\n"
	exit
endif

# Constants
set INPUT_DIR = $argv[1]
set OUTPUT_DIR = $argv[2]
set THUMB_DIR = $OUTPUT_DIR/thumbs
set OUTPUT_FORMAT = jpg
set OUTPUT_HTML = pic_name_xx.html
set VALID_INPUT_TYPES = ".png .gif .tiff"

# Variables for creating HTML file
set tr_pictures = ""
set tr_dimensions = ""
set tr_date = ""

# Check for input directory
if((! -r $INPUT_DIR)) then
  printf "FATAL: Failed to find/read input directory\n"
	exit
endif

printf "Starting generation..\n"

# Create output directory if needed
if ((! -d $OUTPUT_DIR) || (! -d $THUMB_DIR)) then
	printf "Creating output directories\n"
	mkdir -p $THUMB_DIR
endif

# For each argument (pattern)
@ i = 3
while ($i <= $#argv)
	set pattern = "$argv[$i]"	
	@ i = $i + 1

	# Make sure pattern is valid (Ex: Not *.pgn, etc)
	@ valid = 0
	foreach str ($VALID_INPUT_TYPES)
		if("$pattern" =~ [^.]*"$str") then
			@ valid = 1
			break
		endif
	end

	# If it didn't match any accepted file formats, skip it
	if($valid == 0) then
		printf "\nPattern: $pattern is invalid, skipping...\n"
		continue
	endif

	printf "\nPattern: $pattern\n"

	# For each file matching the pattern
	foreach in_file (`find $INPUT_DIR -name "$pattern"`)
    # Make sure file is readable
    if((! -r $in_file)) then
      printf "\tERR - Could not read $in_file\n"
      continue
    endif
		# Generate new file name (Ex: ./a/b/myFile.jpg -> MYFILE)
		set file_name_upper = `basename "$in_file" | cut -f 1 -d '.' | tr '[a-z]' '[A-Z]'`
		set new_base_file = $OUTPUT_DIR/$file_name_upper.$OUTPUT_FORMAT
		set new_thumb_file = $THUMB_DIR/$file_name_upper"_thumb".jpg

		# Generate the new files if base and thumb don't already exist
		if ((! -f $new_base_file) && (! -f $new_thumb_file)) then
			convert $in_file $new_base_file
			convert $in_file -resize 200x200 $new_thumb_file
			printf "\tOK - Converted $in_file.\n"

			# Update the variables used to create the HTML page
			set tr_pictures = "$tr_pictures <td><a href=$new_base_file><img src=$new_thumb_file /></a></td>"
			set tr_dimensions = "$tr_dimensions <td align="center">`identify -format '%wx%h' $new_thumb_file`</td>"
			set tr_date = "$tr_date <td align="center">`stat -c %y $in_file | cut -f1 -d ' '`</td>"
		else
			printf "\tOK - Not converting $in_file : base and/or thumb already exist.\n"
		endif
	end
end

# Build the HTML file
echo ""
echo -n "Please enter a theme: "
set theme = $<
set date_and_day = `date "+%A, %m/%d/%y"`
set html = "<html><head><title>Test Images</title></head><body>\
	<table border = 2>\
	<tr>$tr_pictures</tr>\
	<tr>$tr_dimensions</tr>\
	<tr>$tr_date</tr>\
	</table>\
	<p>Theme: $theme</p>\
	<p> Date & day: $date_and_day</p>\
	</body></html>"

# Output the HTML file
echo $html > $OUTPUT_HTML

printf "\nDone! Check for errors.\n"
