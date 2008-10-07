#!/usr/bin/env bash

TOOLS_DIR=tools
REACTIVISION_ZIP=reacTIVision-1.4pre2-src.zip
REACTIVISION_DIR=reacTIVision-1.4pre2-src
SIMULATOR_ZIP=TUIO_Simulator-1.3.zip
SIMULATOR_DIR=TUIO_Simulator
LIBDC=libdc1394-2.0.2.tar.bz2

function usage()
{
	echo "Usage: launch.sh <simulator|reactivision|ladspa>"
	exit
}

function reactivision()
{
	pushd . > /dev/null

	cd $TOOLS_DIR/reactivision/
	if [ ! -d $REACTIVISION_DIR ]; then

		# install libdc
		tar xjf $LIBDC
		cd `basename $LIBDC .tar.bz2`
		./configure && make && sudo make install
		cd ..

		# install reactivision
		unzip $REACTIVISION_ZIP
		cd $REACTIVISION_DIR/linux
		make
		cd ../..
	fi

	cd $REACTIVISION_DIR/linux
	./reacTIVision

	popd >/dev/null
}

function simulator()
{
	pushd . > /dev/null

	cd $TOOLS_DIR/tuio/
	if [ ! -d $SIMULATOR_DIR ]; then
		unzip $SIMULATOR_ZIP
	fi

	cd TUIO_Simulator
	java -jar TuioSimulator.jar

	popd >/dev/null
}

function ladspa()
{
	echo 'In;Out;UID;Name;Description;'
	for foo in /usr/lib/ladspa/*.so; do
		echo -n `analyseplugin $foo | grep input | grep audio | wc -l`
		echo -n ";"
		echo -n `analyseplugin $foo | grep output | grep audio | wc -l`
		echo -n ";"
		echo `analyseplugin -l $foo|sed -r 's/^([[:alnum:]|_]+)[[:space:]]+([[:xdigit:]]*)[[:space:]]+(.*)/\2;\1;\3;/g'`;
	done
}

case "$1" in
	"simulator")
		simulator;;
	"reactivision")
		reactivision;;
	"ladspa")
		ladspa;;
	*)
		usage;;
esac
