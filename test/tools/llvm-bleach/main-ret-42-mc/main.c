__attribute__((noinline)) unsigned long long foo() { return 42; }

int main() { return foo(); }
