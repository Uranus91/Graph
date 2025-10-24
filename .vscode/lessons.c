#include <stdio.h>

int is_range(double x, double a, double b) {
    if (x > a && x < b)
        return 1;
    return 0;
}

int main(void)
{
    double x, a = -2.5, b = 3.5;
    while (scanf("%f", &x) == 1) {
        if (is_range(x, a, b) == 0)
            printf("%.2f", x);
    }
        
    

    return 0;
}