#include <stdio.h>
#include <stdint.h>
#include "smalloc.h"

int main ()
{
	void *p1;

	smdump() ;

	p1 = smalloc(8000) ; 
	printf("smalloc(8000):%p\n", p1) ; 
	smdump() ;
}
// 페이지보다 큰 메모리 할당시 처리를 위한 테스트케이스