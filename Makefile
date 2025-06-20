
install:
	sudo apt-get install libsfml-dev
	sudo apt-get install build-essential

compile:
	g++ game.cpp -o game.exe -lsfml-graphics -lsfml-window -lsfml-system

run:
	./game.exe
