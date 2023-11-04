CXXFLAGS = -Wall -g

riscv_sim: main.o sim.o
	$(CXX) -o $@ $^

main.o: main.cc sim.hpp common.hpp
sim.o: sim.cc sim.hpp common.hpp

clean:
	$(RM) riscv_sim main.o sim.o
