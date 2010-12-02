#include "controller.h"

int main(int argc, char *argv[]){
	Controller *c = new Controller(argc, argv);
	c->run();
	delete c;
	return 0;
}