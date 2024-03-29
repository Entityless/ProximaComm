TARGET = test_proxima
CC = mpicc
CPP = mpiicpc
LD  = mpiicpc

CCFLAGS =  -O2 -qopenmp
CPPFLAGS =  -O2 -std=c++17 -qopenmp
LDFLAGS  =  -O2 -std=c++17 -qopenmp

CPPOBJ = main.o proxima_weird_locker.o

VPATH = ./proxima/

$(TARGET): $(COBJ) $(CPPOBJ)
	$(LD) $(LDFLAGS) $(COBJ) $(CPPOBJ) -o $(TARGET) 

$(COBJ) : %.o : %.c
	$(CC) $(CCFLAGS) -c -o $@ $<

$(CPPOBJ) : %.o : %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

run:
	set -x
	./$(TARGET)
	set +x

#-------------------------------------*
.PHONY : clean clear
clean:
	-rm -rf $(TARGET) $(COBJ) $(CPPOBJ)
