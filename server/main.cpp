#include <functional>
#include <string>
#include <iostream>

struct helper
{
	template<class F, class... Args>
	void spawn(F &&f, Args &&...args)
	{
		auto x = std::bind(f, std::forward<Args>(args)..., *this, std::placeholders::_1);
		x(5);
	}
};

void my_func(const std::string &arg1, helper &hlp, int arg2)
{
	std::cout << arg2 << "\n";
}

int main()
{
	helper hlp;
	hlp.spawn(my_func, "hi");
}