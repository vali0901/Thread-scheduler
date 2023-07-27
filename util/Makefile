CC = gcc

.PHONY: build
build:libscheduler.so

libscheduler.so: data_structures.o so_scheduler.o
	$(CC) -fPIC -lpthread -shared data_structures.o so_scheduler.c -o libscheduler.so

data_strctures.o: data_structures.c data_structures.h
	$(CC) -fPIC -o $@ -c $<

so_scheduler: so_scheduler.c so_scheduler.h
	$(CC) -fPIC -o $@ -c $<

.PHONY: clean
clean:
	rm *.o libscheduler.so