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
	cd test; make test;
