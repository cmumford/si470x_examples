
CLANG_FORMAT=clang-format

SOURCE_FILES = \
	  example/mgos/main.c \
		example/unix/rdsdisplay.cc \
		util/oda_decode.c \
		util/oda_decode.h \
		util/rds_util.c \
		util/rds_util.h

.PHONY: format
format:
	${CLANG_FORMAT} -i ${SOURCE_FILES}

docs: doxygen.conf Makefile
	doxygen doxygen.conf

.PHONY: clean
clean:
	rm -rf build docs deps memcheck.log

build:
	mkdir build

build/Makefile: build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug ..

build/rdsdisplay: build/Makefile
	make --directory=build -j

.PHONY: apps
apps: build/rdsdisplay

.PHONY: tags
tags:
	ctags --extra=+f --languages=+C,+C++ --recurse=yes --links=no

.PHONY: memcheck
memcheck: build/rdsdisplay
	valgrind --log-file=memcheck.log build/rdsdisplay ../rds-spy-logs/Germany
