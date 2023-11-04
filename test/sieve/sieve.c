void sieve(char p[], int n) {
    for (int i = 0; i < n; i++) {
        p[i] = -1;
    }
    p[0] = 0;
    p[1] = 0;
    for (int i = 2; i * i < n; i++) {
        if (!p[i]) continue;
        for (int j = i * i; j < n; j += i) {
            p[j] = 0;
        }
    }
}

int main() {
    char p[100];
    sieve(p, 100);
}
