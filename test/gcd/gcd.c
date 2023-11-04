// without this attribute, gcc inlines this call inside main,
// and we want to see that the function call mechanism works
__attribute__((noinline))
static int gcd(int a, int b) {
    // assume a >= 0 && b >= 0
    // make sure a >= b by swapping if necessary
    if (a < b) {
        int t = a;
        a = b;
        b = t;
    }

    while (b > 0) {
        // keep subtracting b from a until a < b
        while (a >= b) {
            a -= b;
        }
        // swap a and b to maintain a > b
        int t = a;
        a = b;
        b = t;
    }

    // now b is 0, and a is the gcd
    return a;
}

int main() {
    // declare these variables volatile to force the compiler
    // to emit load and store instructions, so we can make
    // sure those instructions work too
    volatile int m = 13, n = 37;
    int out = gcd(m, n);
}
