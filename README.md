# Compiler Exploring
## Introduction
This is probably one of the most basic compiler explorer investigation that can be made. It started when I was browsing through the [Embedded Artistry](https://embeddedartistry.com)'s resources [repo](https://github.com/embeddedartistry/embedded-resources/) and found a file called `bad.c`.

## The Setup
In that file there was a simple example of undefined behavior of using unitialized variables.
```c
void foo(void)
{
	int a = 5;
	int b;
}

void bar(void)
{
	int x;
	printf("%d\n", x++);
}

int main(void)
{
	foo();
	bar();
	bar();

	return 0;
}
```

Looking at the file you can see that when we call the `bar` function the stack will be populated by the data stored by the `foo` procedure, and the output should look something like this:
```shell
$ gcc bad.original.c; ./a.out
5
6
```
## The Problem 
As I was playing around with the file I wrote down a copy of the `bar` function with the difference of adding another variable (`y = 8`) and not incrementing them.
```c
void bar2() {
  int x;
  int y;
  printf("%d %d\n", x, y);
}
``` 

I was surprised to see that the output changed slightly:
```shell
$ gcc bad.c; ./a.out         
9 6
10 7
7 10
```
Why when we printed from `bar2` we get a reversed order? That made me somewhat curious and the fact that when we compile the same program with the clang compiler we get the "expected" out put. So I opened up compiler explorer and looked at some of the assembly output in [Compiler Explorer](https://godbolt.org/z/PpM3Lw):
```asm
foo:
        push    rbp
        mov     rbp, rsp
        mov     DWORD PTR [rbp-4], 5
        mov     DWORD PTR [rbp-8], 8
        nop
        pop     rbp
        ret
.LC0:
        .string "%d %d\n"
bar:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        add     DWORD PTR [rbp-4], 1
        add     DWORD PTR [rbp-8], 1
        mov     edx, DWORD PTR [rbp-4]
        mov     eax, DWORD PTR [rbp-8]
        mov     esi, eax
        mov     edi, OFFSET FLAT:.LC0
        mov     eax, 0
        call    printf
        nop
        leave
        ret
bar2:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        mov     edx, DWORD PTR [rbp-8]
        mov     eax, DWORD PTR [rbp-4]
        mov     esi, eax
        mov     edi, OFFSET FLAT:.LC0
        mov     eax, 0
        call    printf
        nop
        leave
        ret
main:
        push    rbp
        mov     rbp, rsp
        call    foo
        call    bar
        call    bar
        mov     eax, 0
        call    bar2
        mov     eax, 0
        pop     rbp
        ret

```
First step is to look at the `call printf`. Knowing that the 64-bit calling Linux convention passes arguments through regiters in the following order:  `rdi, rsi, rdx, r10, r9, r8`.  

So we see that the main difference between `bar` and `bar2` is the fact that `[rbp-8]` comes before `[rbp-4]`, while the order of the registers is manteined. Indicating that in the second function the stack is read in reverse order. And when we look at the clang output we see that the order is preserved. From that we conclude that simply incrementing the variables instead of immediately passing them as arguments causes the gcc compiler to drop the "normal" ordering when reading the stack. 

## Conclusion
It is really interesting that this supposedly inconsequential change can cause such behaviors in the code. As a bonus quirk of gcc we can see that when we call `bar2` we clear the `eax` register. But when we just call `bar` three times we get no clearing. While clang does the clearing independenly of the order.

Clang output in [Compiler Explorer](https://godbolt.org/z/S-8jNP):
```asm
foo:                                    # @foo
        push    rbp
        mov     rbp, rsp
        mov     dword ptr [rbp - 4], 5
        mov     dword ptr [rbp - 8], 8
        pop     rbp
        ret
bar:                                    # @bar
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        mov     eax, dword ptr [rbp - 4]
        add     eax, 1
        mov     dword ptr [rbp - 4], eax
        mov     ecx, dword ptr [rbp - 8]
        add     ecx, 1
        mov     dword ptr [rbp - 8], ecx
        movabs  rdi, offset .L.str
        mov     esi, eax
        mov     edx, ecx
        mov     al, 0
        call    printf
        add     rsp, 16
        pop     rbp
        ret
bar2:                                   # @bar2
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        mov     esi, dword ptr [rbp - 4]
        mov     edx, dword ptr [rbp - 8]
        movabs  rdi, offset .L.str
        mov     al, 0
        call    printf
        add     rsp, 16
        pop     rbp
        ret
main:                                   # @main
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        mov     dword ptr [rbp - 4], 0
        call    foo
        call    bar
        call    bar
        call    bar2
        xor     eax, eax
        add     rsp, 16
        pop     rbp
        ret
.L.str:
        .asciz  "%d %d\n"

```

Let me end by reminding everyone that the use of uninitalized variables is clear undefined behavior and the compiler may do with that whatever it thinks is correct. Such clear violation of basic clean code conventions should easily be reported by a code sanitizer such as cppcheck. And note that simply turning on optimizations with `-O1` removed all the behavior as the `foo` function was optimized away as it produced no output.   