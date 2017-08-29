#include <stdio.h>
#include <math.h>

#if 0
#define SQRT_64(X) \
{ \
	__asm__ volatile( \
		vmov.f64  d0, r0,r1 \
		vsqrt.f64 d0, d0 \
		vmov.f64  r0,r1, d0 \
		bx        lr; \
	); \
}
#endif

#define SQRT_32(X) \
{ \
__asm__ volatile( \
	vmov.f32  s0, r0 \
	vsqrt.f32 s0, s0 \
	vmov.f32  r0, s0 \
	bx        lr; \
); \
}

int in = 9;
int out = 8; 

float test_sqrt32()
{
	float x = 2.0;
	float z = 0.0;//SQRT_32(x);
	printf("in = %d, out = %d\n", in, out);
	__asm__ __volatile__( 
		"movl  %1, %0"
		"sqrt %1, %1" 
		"movl  %0, %1" 
		"bx        lr" 
		: "=r" (z) 
		: "r" (x)
	); 
	printf("[32]z = %f\n", z);
	return z;
}

float test_sqrt()
{
	float x = 2.0;
	float z = sqrt(x);
	printf("z = %f\n", z);
	return z;
}

int main()
{
	float x = 2.0;
	float z = 0;
	printf("z = %f\n", z);
	test_sqrt();
	test_sqrt32();
	return (int)z;
}

