__attribute__((noinline))
static float my_fabs(float x) {
    return x < 0 ? -x : x;
}

__attribute__((noinline))
static float my_sqrt(float x) {
    float out = 1.0f;
    while (my_fabs(out * out - x) / x > 1e-6f) {
        out = (out + x / out) / 2.0f;
    }
    return out;
}

int main() {
    float f = my_sqrt(2.0f);
}
