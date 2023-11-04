__attribute__((noinline))
int gcd(int a, int b) {
    if (a < b) {
        int t = a;
        a = b;
        b = t;
    }
    while (b > 0) {
        while (b <= a) {
            a -= b;
        }
        int t = a;
        a = b;
        b = t;
    }
    return a;
}

int main() {
    volatile int m = 13, n = 37;
    int out = gcd(m, n);
}
