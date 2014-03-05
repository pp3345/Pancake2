#!/bin/bash

PANCAKE_SOURCES="ConfigurationParser/grammar.y ConfigurationParser/scanner.l ConfigurationParser/PancakeConfigurationParser.c  ConfigurationParser/scanctx.c  ConfigurationParser/strbuf.c Pancake.c PancakeLogger.c PancakeDateTime.c PancakeDebug.c PancakeConfiguration.c PancakeNetwork.c PancakeModules.c PancakeWorkers.c PancakeScheduler.c"

for dir in */;
do
	if [ -f $dir/module.m4 ];
	then
		AUTOCONF_SOURCE+=`cat $dir/module.m4`;
		AUTOCONF_SOURCE+=$'\n';
		
		for file in $dir*.c
		do
			echo "Added file $file"
			PANCAKE_SOURCES+=" $file"
		done
	fi
done

cp build/configure.ac configure.ac
cp build/Makefile.am Makefile.am

echo "Pancake_SOURCES = $PANCAKE_SOURCES" >> Makefile.am

REPLACEMENT=$(echo "$AUTOCONF_SOURCE" | sed -e 's/[\/&]/\\&/g' | sed 's/$/\\n/' | tr -d '\n')
sed -i "s/MODULES_AUTOCONF/$REPLACEMENT/g" configure.ac

echo "autoreconf Pancake"
autoreconf --force --install