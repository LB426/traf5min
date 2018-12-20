INC += -I/usr/local/pgsql/include
LNK += -L/usr/local/pgsql/lib -lpq

traf5min : main.o
		g++ -o traf5min main.o $(LNK)

main.o : main.cpp main.h libpq-fe.h
		g++ -c $(INC) main.cpp

clean :
		rm traf5min main.o
						
