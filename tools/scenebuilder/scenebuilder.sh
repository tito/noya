#!/usr/bin/env bash

#
# Default configuration
#
DEF_FILENAME=test
DEF_TITLE="Test scene"
DEF_AUTHOR="$USER"
DEF_BPM=125

#
# Dynamic configuration
#
DATE=`date +'%d.%m.%Y'`
BASEPATH=$(realpath $(dirname $0))
NOYAPATH=$(realpath $BASEPATH/../..)
TEMPLATE="$BASEPATH/template.cfg"


#
# Variables
#
FILENAME=
TITLE=
AUTHOR=
BPM=
WPATH=
FORCE=0
COPYAUDIO=0

echo "Noya scene builder, ver 0.1 - by Mathieu Virbel <tito@bankiz.org>"
echo ""

usage() {
	echo "Usage: scenebuilder.sh [options] <path>"
	echo " -a, --author <author>      Scene author"
	echo " -b, --bpm <bpm>            Set BPM (beat per minutes)"
	echo " -c, --copy                 Copy audio wave in directory (deprecated)"
	echo " -h, --help                 Show help"
	echo " -o, --output <filename>    Output filename"
	echo " -t, --title <title>        Scene title"
	echo " -f, --force                Force write if exist"
}

#
# Test template.cfg
#

if [ ! -r "$TEMPLATE" ]; then
	echo "Unable to found template.cfg in $TEMPLATE"
	exit
fi


#
# Read parameters
#

while [ -n "$1" ]; do
	case "$1" in
		"-h" | "--help")
			usage
			;;
		"-a" | "--author")
			shift
			AUTHOR="$1"
			;;
		"-b" | "--bpm")
			shift
			BPM="$1"
			;;
		"-o" | "--output")
			shift
			FILENAME="$1"
			;;
		"-t" | "--title")
			shift
			TITLE="$1"
			;;
		"-f" | "--force")
			FORCE=1
			;;
		"-c" | "--copy")
			COPYAUDIO=1
			;;
		*)
			WPATH="$1"
			break
			;;
	esac
	shift
done



#
# Verify parameters
#

if [ "X$WPATH" = "X" ]; then
	usage
	exit
fi

if [ ! -d "$WPATH" ]; then
	echo "Invalid directory: $WPATH"
	exit
fi

if [ "X$FILENAME" = "X" ]; then
	echo -n "Scene filename [$DEF_FILENAME]: "
	read FILENAME

	if [ "X$FILENAME" = "X" ]; then
		FILENAME="$DEF_FILENAME"
	fi
fi

if [ "X$TITLE" = "X" ]; then
	echo -n "Title of scene [$DEF_TITLE]: "
	read TITLE

	if [ "X$TITLE" = "X" ]; then
		TITLE="$DEF_TITLE"
	fi
fi

if [ "X$AUTHOR" = "X" ]; then
	echo -n "Author [$DEF_AUTHOR]: "
	read AUTHOR

	if [ "X$AUTHOR" = "X" ]; then
		AUTHOR="$DEF_AUTHOR"
	fi
fi

if [ "X$BPM" = "X" ]; then
	echo -n "Beat per minutes [$DEF_BPM]: "
	read BPM

	if [ "X$BPM" = "X" ]; then
		BPM="$DEF_BPM"
	fi
fi


#
# Create output directory
#

OUTPUT_DIRECTORY="$NOYAPATH/scenes/$FILENAME"
if [ -d "$OUTPUT_DIRECTORY" ]; then
	echo "Warning, scene already exists with this name !"
	if [ "X$FORCE" = "X0" ]; then
		echo "Please remove $OUTPUT_DIRECTORY before generate scene."
		exit
	fi

	rm -rf $OUTPUT_DIRECTORY
fi
mkdir -p $OUTPUT_DIRECTORY
if [ ! -d "$OUTPUT_DIRECTORY" ]; then
	echo "Error, unable to create $OUTPUT_DIRECTORY"
	exit
fi

#
# Create output file from template
#

OUTPUT_FILENAME="$OUTPUT_DIRECTORY/$FILENAME.cfg"
touch $OUTPUT_FILENAME
if [ ! -w "$OUTPUT_FILENAME" ]; then
	echo "Error, unable to create $OUTPUT_FILENAME"
	exit
fi

cat $TEMPLATE | sed "s/\$TITLE/$TITLE/g" | sed "s/\$AUTHOR/$AUTHOR/g" | sed "s/\$BPM/$BPM/g" | sed "s/\$DATE/$DATE/g" > $OUTPUT_FILENAME

#
# Copy files
#
idx=0
IFS='
'
for wavfile in `find "$WPATH" -iname *.WAV`; do
	wavname=`basename $wavfile`

	echo "Map actor $idx to $wavname"

	if [ $COPYAUDIO -eq 1 ]; then
		cp "$wavfile" "$OUTPUT_DIRECTORY/$wavname"
	fi

	echo "		{" >> $OUTPUT_FILENAME
	echo "			id = $idx;" >> $OUTPUT_FILENAME
	echo "			class = \"default\";" >> $OUTPUT_FILENAME
	echo "			file = \"$wavname\";" >> $OUTPUT_FILENAME
	echo "		}," >> $OUTPUT_FILENAME

	idx=$((idx+1))
done
echo "	{});" >> $OUTPUT_FILENAME
echo "};" >> $OUTPUT_FILENAME

if [ $COPYAUDIO -eq 1 ]; then
	echo
	echo "Don't forget to update your database !"
	echo "Do: noya -i scenes"
	echo
fi

# complete !
echo 
echo "Build complete, $idx actors imported."
echo "Scene saved to $OUTPUT_DIRECTORY"
echo


