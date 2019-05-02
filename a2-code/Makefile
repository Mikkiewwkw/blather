CFLAGS = -Wall -g
CC     = gcc $(CFLAGS)

# commando is a program, depends on three files, is default target
blather : server_funcs.o simpio.o bl_client.o util.o bl_server.o
	$(CC) -o blather server_funcs.o bl_client.o util.o bl_server.o
	@echo blather is ready

bl_server.o : bl_server.c blather.h
	$(CC) -c $<

server_funcs.o : server_funcs.c blather.h
	$(CC) -c $<

simpio.o : simpio.c blather.h
	$(CC) -c $<

bl_client.o : bl_client.c blather.h
	$(CC) -c $<

util.o : util.c blather.h
	$(CC) -c $<

## TEST TARGETS
TEST_PROGRAMS = test_blather.sh test_blather_data.sh normalize.awk cat_sig.sh filter-semopen-bug.awk

test : test-blather

make test-blather : bl_client bl_server $(TEST_PROGRAMS)
	chmod u+rx $(TEST_PROGRAMS)
	./test_blather.sh

clean-tests :
	cd test-data && \
	rm -f *.*

clean-tests :
	rm -f test_functions test-data/*.tmp
