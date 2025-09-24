all: pr4

pr4:
	gcc -Wall -o pr4 pr4.c -lcrypto && gcc -Wall -o pr4_p pr4_p.c -lcrypto -lpthread
clean:
	rm pr4 && rm pr4_p
test:
	time -p ./pr4_p transactions2.csv 8
test_valgrind:
	valgrind --leak-check=full ./pr4_p transactions2_short.csv 8
