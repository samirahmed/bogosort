
clean:
	cd lib; make clean;
	cd ..;
	cd client; make clean;
	cd ..;
	cd server; make clean;

all:
	cd lib; make;
	cd ..;
	cd client; make;
	cd ..;
	cd server; make;

