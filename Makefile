# Uncomment lines below if you have problems with $PATH
#SHELL := /bin/bash
#PATH := /usr/local/bin:$(PATH)

all : build uploadota

build :
		pio -f -c vim run

upload :
		pio -f -c vim run --target upload

clean :
		pio -f -c vim run --target clean

cleanall :
		pio -f -c vim run --target cleanall

program :
		pio -f -c vim run --target program

uploadfs :
		pio -f -c vim run --target uploadfs

uploadota :
		pio -f -c vim run --target upload --upload-port 10.201.1.27

update :
		pio -f -c vim update

list-targets :
		pio -f -c vim run --list-targets
