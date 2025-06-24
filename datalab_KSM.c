/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* 
 * bitNor - ~(x|y) using only ~ and & 
 *   Example: bitNor(0x6, 0x5) = 0xFFFFFFF8
 *   Legal ops: ~ &
 *   Max ops: 8
 *   Rating: 1
 */
int bitNor(int x, int y) {
  return ((~x)&(~y));
}
/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (least significant) to 3 (most significant)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>

 */
int getByte(int x, int n) {
     // n을 3 비트만큼 시프트한 후, x에서 추출한 바이트를 반환
    x = (x >> (n << 3));
    return x & 0xFF;
}
/* 
 * bitMask - Generate a mask consisting of all 1's 
 *   lowbit and highbit
 *   Examples: bitMask(5,3) = 0x38
 *   Assume 0 <= lowbit <= 31, and 0 <= highbit <= 31
 *   If lowbit > highbit, then mask should be all 0's
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int bitMask(int highbit, int lowbit) {	
	int lowmask = ~(~0 << lowbit); 
    int highmask1 = ~0 << highbit;
    int highmask2 = ~(1 << highbit);

    // lowmask와 (highmask1 & highmask2)를 OR 연산하여 비트 마스크 생성 후, NOT 연산으로 반전
    return ~(lowmask | (highmask1 & highmask2));
}
/* 
 * bang - Compute !x without using !
 *   Examples: bang(3) = 0, bang(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int bang(int x) {
    // x와 -x를 OR 연산하여 부호 비트 확인
    x = (x | (~x+1)) >> 31;

    // 부호 비트에 1을 더하고 결과를 반환
    return (((x | (~x+1)) >> 31) + 1);
}
/*
 * bitParity - returns 1 if x contains an odd number of 0's
 *   Examples: bitParity(5) = 0, bitParity(7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int bitParity(int x) {
    // 비트를 오른쪽으로 시프트하면서 XOR 연산을 수행하여 0의 개수 판별
	x ^= x >> 1;
	x ^= x >> 2;
	x ^= x >> 4;
	x ^= x >> 8;
	x ^= x >> 16;

    // 가장 낮은 비트를 추출하여 반환
	return x & 1;
}
/* 
 * TMax - return maximum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {
	int n = 1 << 31; // 비트 이동
	int result = ~n; // 'n'의 비트를 반전시켜 최대 양수 정수 생성
	return result;
}
/* 
 * isNegative - return 1 if x < 0, return 0 otherwise 
 *   Example: isNegative(-1) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNegative(int x){
	int result;
	x = (x>>31); // 입력값 x의 부호 비트를 추출 (0 또는 -1)
	result = x&1; // 부호 비트와 1을 비교하여 음수면 1을 반환, 아니면 0을 반환
	return result;
}
/* 
 * fitsBits - return 1 if x can be represented as an 
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
	int mask = x >> 31; // 입력값 x의 부호 비트를 추출
	int n_minus_1 = n + ~0; // n - 1을 계산

    // x를 n 비트로 오른쪽 시프트하고, 이를 부호 비트와 비교하여
    // 같으면 1을 반환 (x가 n 비트로 표현 가능), 다르면 0을 반환 (불가능)
	return !((x >> n_minus_1) ^ mask);
}
/* 
 * dividePower2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: dividePower2(15,1) = 7, dividePower2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int dividePower2(int x, int n) {
	int shift = x >> 31; // 입력값의 부호 비트 추출 (0 또는 -1)
	int result = (~0) + (1 << n);	// 2^n - 1 값을 계산

    // 입력값이 음수일 경우 2^n - 1 값을 더하여 반올림 효과 구현,
    // 그렇지 않으면 그대로 반환
	return ((shift & result) + x) >> n;
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
     // 입력값 x를 32비트로 확장하여 사용 (부호 확장)
	x = x | (x << 16);
	x = x | (x << 8);
	x = x | (x << 4);
	x = x | (x << 2);
	x = x | (x << 1);

	x = x >> 31; // x의 부호 비트 추출

    // x가 0인 경우 (거짓), y를 반환하고, 그렇지 않으면 (참) z를 반환
	return ((~x & z) | (x & y));
}
/*
 * ezThreeFourths - multiplies by 3/4 rounding toward 0,
 *   Should exactly duplicate effect of C expression (x*3/4),
 *   including overflow behavior.
 *   Examples: ezThreeFourths(11) = 8
 *             ezThreeFourths(-9) = -6
 *             ezThreeFourths(1073741824) = -268435456 (overflow)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 3
 */
int ezThreeFourths(int x) {
	int n = (x << 1) + x; // 입력값을 3배로 곱함
  	int std = !!(n & (0x1 << 31)); // 입력값의 부호 비트 추출

    // 입력값이 음수이면 1을 더하여 반올림 효과를 구현
 	int t = n + (0x3 & (std | std << 1));
       	return t >> 2; // 4로 나누어 3/4 값을 구함
}
/* 
 * signMag2TwosComp - Convert from sign-magnitude to two's complement
 *   where the MSB is the sign bit
 *   Example: signMag2TwosComp(0x80000005) = -5.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 4
 */
int signMag2TwosComp(int x) {
	int sign = x >> 31; // 입력값의 부호 비트 추출 (0 또는 -1)
	int xsign = sign << 31; // 부호 비트를 31번 비트로 확장
	int xwsign = x ^ xsign; // 부호 비트를 제외한 비트 유지

    // 부호가 양수인 경우 (0), 그대로 반환하고, 부호가 음수인 경우 (-1)에는
    // 2의 보수를 취해 부호를 반전하고 1을 더하여 결과 반환
	int result = (xwsign ^ sign) + (sign & 1);
	return result;
}
/* 
 * floatAbsVal - Return bit-level equivalent of absolute value of f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument..
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned floatAbsVal(unsigned uf) {
	unsigned a = (uf >> 23) & 0xFF; // 지수 부분을 추출하고 8비트로 마스킹
	unsigned b = uf << 9; // 나머지 비트를 추출

    // 입력값이 NaN인 경우 (지수 부분이 모두 1이고 나머지 비트가 0이 아닌 경우) 입력값 그대로 반환
        if (a == 0xFF && b != 0x00){
		return uf;
	}

     // 입력값의 부호 비트를 제외한 비트를 유지하여 절댓값을 반환
	return (uf & ~(1<<31));
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
	int sgn = uf >> 31; // 부호 비트 추출
        int exponent = ((uf & 0x7F800000u) >> 23) - 127; // 지수 비트 추출 및 바이어스 제거
        int fraction = (uf & 0x007FFFFFu) | 0x00800000u; // 유효 부분 추출 및 정규화 비트 추가
        
        // 지수가 음수인 경우 (0 미만인 경우) 0 반환
        if (exponent < 0)
                return 0;

        // 지수에 따라 유효 부분을 적절하게 시프트하여 정수 부분을 얻음
        if (exponent > 23)
                fraction = fraction << (exponent - 23);
        else
                fraction = fraction >> (23 - exponent);

	// 지수가 31 이상인 경우 (범위를 벗어난 경우) 가장 작은 음수값 반환
        if (exponent >= 31)
                return 0x80000000u;


         // 양수일 경우 정수 부분 반환, 음수일 경우 2의 보수를 취하여 반환
        if (sgn == 0)
                return fraction;
        else
                return ~fraction + 1;
}

/* 
 * floatScale4 - Return bit-level equivalent of expression 4*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale4(unsigned uf) {
    int count = 2; // 반복 횟수를 나타내는 변수를 초기화 (4배 스케일링을 위해 2번 반복), while문 위에 선언시 parse error 맨 윗부분에 작성
    unsigned int mask = ~(1 << 31); // 부호 비트를 제외한 모든 비트를 1로 설정하는 마스크 생성
    unsigned int f = mask & uf; // 입력값의 부호 비트를 제외한 값을 f에 저장
    unsigned int fracMask = 0xFFFFFFFFu + (1 << 23); // 유효 부분(fraction)을 나타내는 비트를 모두 1로 설정하는 마스크 생성
    unsigned int expMask = 0xFF; // 지수 부분(exponent)을 나타내는 비트를 모두 1로 설정하는 마스크 생성
    unsigned int s = uf >> 31; // 입력값의 부호 비트를 s에 저장

    unsigned int exponent = f >> 23; // 입력값에서 지수 부분 추출
    unsigned int fraction = fracMask & uf; // 입력값에서 유효 부분 추출

     // 입력값이 NaN인 경우, 입력값 그대로 반환
    if ((exponent == 0xFF) && (fraction != 0)) {
	    return uf;
    }
    // 입력값을 4배 스케일링하는 반복 루프
        while (count>0) {
        if (exponent != 0) { // 지수가 0이 아닌 경우
            if (exponent != 0xFF) {
                exponent = exponent + 1; // 지수를 1 증가시킴 (4배 스케일링)
            }
            if (exponent == 0xFF) {
                fraction = 0; // 지수가 오버플로우되면 유효 부분을 0으로 설정
            }
        } else { // 지수가 0인 경우
            if (fraction & (1 << 22)) {
                fraction = fraction << 1; // 유효 부분을 왼쪽으로 시프트
                exponent = 1; // 지수를 1로 설정
            } else {
                fraction = fraction << 1; // 유효 부분을 왼쪽으로 시프트
                exponent = 0; // 지수를 0으로 설정
            }
        }
        count = count - 1; // 반복 횟수를 감소시킴
    }

    
    fraction = fraction & fracMask; // 유효 부분 마스크를 적용하여 유효 부분 정리
    exponent = exponent & expMask; // 지수 부분 마스크를 적용하여 지수 부분 정리
    return (s << 31) | ((exponent) << 23) | fraction; // 부호, 지수, 유효 부분을 조합하여 결과 반환
}

