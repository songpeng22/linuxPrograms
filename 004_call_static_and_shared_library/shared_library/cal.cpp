#include <stdio.h>

#include "add.h"
#include "sub.h"
#include "mul.h"
#include "div.h"

int cal(int operate,int a,int b)
{
	if(0 == operate)
	{
		return add(a,b);
	}

	return -1;
}
