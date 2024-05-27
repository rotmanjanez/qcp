int foo() {
	return __func__[0] == 'f' &&  __func__[1] == 'f' && __func__[2] == 'f' && __func__[3] == 0;
}