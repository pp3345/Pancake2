#!/bin/bash

PANCAKE_SOURCES="Pancake.c PancakeLogger.c PancakeDateTime.c PancakeDebug.c PancakeConfiguration.c PancakeNetwork.c PancakeModules.c"

for dir in */;
do
	if [ -f $dir/module.m4 ];
	then
		AUTOCONF_SOURCE+=`cat $dir/module.m4`;
		AUTOCONF_SOURCE+=$'\n';
		
		for file in $dir*.c
		do
			PANCAKE_SOURCES+=" $file"
		done
	fi
done

cp build/configure.ac configure.ac
cp build/Makefile.am Makefile.am

echo "Pancake_SOURCES = $PANCAKE_SOURCES" >> Makefile.am

REPLACEMENT=$(echo "$AUTOCONF_SOURCE" | sed -e 's/[\/&]/\\&/g' | sed 's/$/\\n/' | tr -d '\n')
sed -i "s/MODULES_AUTOCONF/$REPLACEMENT/g" configure.ac

autoreconf --force --install