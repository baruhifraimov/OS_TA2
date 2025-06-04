# -C changes the dir
## it start . then changes to ./LVL# and return to .

all:
	make -C LVL6
	make -C LVL5
	make -C LVL4
	make -C LVL3
	make -C LVL2
	make -C LVL1

clean:
	make -C LVL6 clean
	make -C LVL5 clean
	make -C LVL4 clean
	make -C LVL3 clean
	make -C LVL2 clean
	make -C LVL1 clean

.PHONY: all clean