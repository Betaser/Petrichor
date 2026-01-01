#ifndef PETRA_H
#define PETRA_H

#include <iostream>
struct Petra {
	std::string say_hello();
	Petra() { 
		std::cout << "initialized Petra\n";
	}
};

#endif