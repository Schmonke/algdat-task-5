build-texthastable:
	gcc texthashtable.c -o texthashtable

build-hashperformance:
	gcc hashperformance.c -o hashperformance

build-all: build-texthashtable build-hashperformance