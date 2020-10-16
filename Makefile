
CLANG_FORMAT=clang-format

SOURCE_FILES = \
	  example/mgos/*.c \
		example/unix/*.cc \
		example/espidf/*.cc \
		util/*.[ch]

.PHONY: format
format:
	${CLANG_FORMAT} -i ${SOURCE_FILES}

docs: doxygen.conf Makefile
	doxygen doxygen.conf

.PHONY: clean
clean:
	rm -rf build docs deps memcheck.log
	platformio -f -c vim run --target clean

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

.PHONY: all
all:
	platformio -f -c vim run

.PHONY: upload
upload:
	platformio -f -c vim run --target upload

.PHONY: program
program:
	platformio -f -c vim run --target program

.PHONY: uploadfs
uploadfs:
	platformio -f -c vim run --target uploadfs

.PHONY: update
update:
	platformio -f -c vim update
