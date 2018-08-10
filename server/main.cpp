#include "client_manager.h"
#include "command"
#include <iostream>

void my_func(const std::string &arg1, helper &hlp, int arg2)
{
	std::cout << arg2 << "\n";
}

int main()
{
	helper hlp;
	hlp.spawn(my_func, "hi");
}