.PHONY: all test clean
all:
	cd lib; make;
	cd ..;
	cd client; make;
	cd ..;
	cd server; make;
	cd ..;
	cd test; make;


clean:
	cd lib; make clean;
	cd ..;
	cd client; make clean;
	cd ..;
	cd server; make clean;
	cd ..;
	cd test; make clean;

test:
	ruby chef.rb ALL -v; cd test; make test;

utest:
	cd test; make test;

itest:
	ruby chef.rb ALL -v;

clean-log:
	cd log; rm -rf *;

clean-docs:
	cd doc; cd full; rm -rf *;
