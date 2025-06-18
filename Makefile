
build:
	g++ game.cpp -o game.exe -lsfml-graphics -lsfml-window -lsfml-system
	chmod +x ./game.exe


run:
	./game.exe
