#pragma once


template<class Data>
class c_MyStack {
public:
	void push();
	void pop();
	int size();
	void clear();

	Data* _Top;
	c_MyStack* nextStackNode;
};