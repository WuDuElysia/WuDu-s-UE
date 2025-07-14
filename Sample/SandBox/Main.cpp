#include<iostream>
#include"Adlog.h"

int main() {
	ade::Adlog::Init();
	ade::Adlog::GetLoggerInstance()->trace("{0},{1},{2}",_FUNCTION_,1,0.14f,true);

	return EXIT_SUCCESS;
}