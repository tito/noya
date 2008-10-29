#!/usr/bin/env bash

WPATH=$1
DATE=`date +'%d.%m.%Y'`


usage() {
	echo "Usage: scenebuilder.sh <path>"
}

if [ "X$WPATH" = "X" ]; then
	usage
	exit
fi

echo -n "Scene filename : "
read FILENAME

if [ "X$FILENAME" = "X" ]; then
	echo "No filename specified... >_<"
	exit
fi

echo -n "Title of scene : "
read TITLE

if [ "X$TITLE" = "X" ]; then
	echo "No title specified... >_<"
	exit
fi

echo -n "Author : "
read AUTHOR

if [ "X$AUTHOR" = "X" ]; then
	echo "No author specified... >_<"
	exit
fi

echo -n "BPM : "
read BPM

if [ "X$BPM" = "X" ]; then
	echo "No bpm specified... >_<"
	exit
fi

# Create directory
mkdir $FILENAME

# Initialize template
OUT="$FILENAME/$FILENAME.ini"
cat template.ini | sed 's/$TITLE/'$TITLE'/g' | sed 's/$AUTHOR/'$AUTHOR'/g' | sed 's/$BPM/'$BPM'/g' | sed 's/$DATE/'$DATE'/g' > $OUT
idx=0

# Copy files
IFS='
'
for foo in `find "$WPATH" -iname *.WAV`; do
	wavname=`basename $foo`

	echo "Import $wavname"

	cp "$foo" "$FILENAME/$wavname"

	echo "scene.act.$idx.object = obj_sample" >> $OUT
	echo "scene.act.$idx.file = \"$wavname\"" >> $OUT
	echo "scene.act.$idx.ctl.angle = volume" >> $OUT
	echo "" >> $OUT

	idx=$((idx+1))
done

# Complete !
echo 
echo "Build complete."
echo "Now, copy your scene in noya/scenes directory :"
echo " mv" $FILENAME "../../scenes/"
echo

