LIBS = -lGL -lGLU -lglut
br-chip8: chip8.cpp main.cpp
	g++ chip8.cpp main.cpp $(LIBS) -o br-chip8
