#!/bin/bash

# exit if any command fails
set -e

# Path to the dir containing UE_4.26, UE_4.27, and UE_5.0 as installed from EGS
# This is used in the path to a command, so it should be in bash form (with forward slashes)
UNREAL_DIR="/d/Games/Epic"
echo UNREAL_DIR: $UNREAL_DIR

# Absolute path to the dir containing the uplugin file, in Windows form (with backslashes)
# This is just the absolute path of .
# First sed: change / to \
# Second sed: fix drive letter (change \c to c:)
UPLUGIN_DIR=`pwd | sed -e 's/\\//\\\\/g' | sed -e 's/\\\\\\(.\\)/\\1:/'`
echo UPLUGIN_DIR: $UPLUGIN_DIR

# https://www.unrealengine.com/en-US/marketplace-guidelines

# 2.6.3.d
# Epic will only build sellers’ plugins against the three latest major engine versions available.
declare -a versions=( "4.26" "4.27" "5.0" )
for v in "${versions[@]}"
do
	echo $v
	# 1.4.2.c
	# Code Plugin products must have a Project Version for every different Supported Engine Version,
	# each with different Project File Links that host different overarching plugin folders,
	# even if they are duplicates with just different values in the “EngineVersion” key of their .uplugin descriptors.
	sed -i -e "s/\"EngineVersion\":.*/\"EngineVersion\": \"$v.0\",/" PMXlsxImporter.uplugin

	# 2.6.3.b Plugins will be distributed with the binaries built by Epic’s compilation toolchain,
	# so sellers must ensure that final debugging has been completed by clicking "Package..."
	# on their plugin in the Plugins windows of the editor to test compilation before sending in a new plugin version.
	# Sellers can also run this command from installed binary builds of each Unreal Engine version
	# they’d like to compile their plugin for:
	"$UNREAL_DIR/UE_$v/Engine/Build/BatchFiles/RunUAT.bat"\
		BuildPlugin\
		-Plugin=$UPLUGIN_DIR\\PMXlsxImporter.uplugin\
		-Package=$UPLUGIN_DIR\\Package\
		-Rocket

	zip -r PMXlsxImporter$v.zip\
		LICENSE\
		PMXlsxImporter.uplugin\
		README.md\
		Content/Python/init_unreal.py\
		Content/Python/install-openpyxl.bat\
		Content/Python/install-openpyxl.sh\
		Content/ImportXlsxWindow.uasset\
		Resources\
		Source
done
