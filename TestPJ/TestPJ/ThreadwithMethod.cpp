#include "stdafx.h"

class Request {
private:
	int id;

public:
	Request(int id)
		: id(id)
	{
	}

	void process() {
		std::cout << "Processing request " << id << std::endl;
	}
};

int main() {
	Request req(10);
	std::thread t{ &Request::process, &req };
	t.join();
}