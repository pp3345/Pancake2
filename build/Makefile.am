bin_PROGRAMS = Pancake

PARSER_PREFIX = PancakeConfigurationParser_yy
BUILT_SOURCES = ConfigurationParser/scanner.c ConfigurationParser/scanner.h ConfigurationParser/grammar.c ConfigurationParser/grammar.h

AM_YFLAGS = -d -p $(PARSER_PREFIX)
AM_LFLAGS = --header-file=scanner.h --prefix=$(PARSER_PREFIX)

