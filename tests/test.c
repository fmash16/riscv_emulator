int fact(int n);
int main() {
    int a = 10;
    /*return fact(a);*/
    return a-11;
}

int fact(int n) {
    if(n==1)
        return n;
    else
        return n * fact(n-1);
}
