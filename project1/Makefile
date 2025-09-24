all: pr1

pr1:
	gcc -Wall -o pr1 pr1.c
clean:
	rm pr1
test:
	./pr1 transactions.csv
test_valgrind:
	valgrind --leak-check=full ./pr1 transactions.csv
