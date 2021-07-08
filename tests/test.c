int fact(int n);
int main() {
    int a = 10;
    return fact(a);
}

int fact(int n) {
    if(n==1)
        return n;
    else
        return n * fact(n-1);
}
