#include <stdio.h>
#include <stdlib.h>

// Task 2: Simple Calculator Program
// This program performs basic arithmetic operations

int main() {
    char operator;
    double num1, num2, result;
    
    printf("=== Simple Calculator ===\n");
    printf("Enter an operator (+, -, *, /): ");
    scanf("%c", &operator);
    
    printf("Enter two numbers: ");
    scanf("%lf %lf", &num1, &num2);
    
    switch(operator) {
        case '+':
            result = num1 + num2;
            printf("%.2lf + %.2lf = %.2lf\n", num1, num2, result);
            break;
        case '-':
            result = num1 - num2;
            printf("%.2lf - %.2lf = %.2lf\n", num1, num2, result);
            break;
        case '*':
            result = num1 * num2;
            printf("%.2lf * %.2lf = %.2lf\n", num1, num2, result);
            break;
        case '/':
            if (num2 != 0) {
                result = num1 / num2;
                printf("%.2lf / %.2lf = %.2lf\n", num1, num2, result);
            } else {
                printf("Error: Division by zero is not allowed!\n");
                return 1;
            }
            break;
        default:
            printf("Error: Invalid operator! Please use +, -, *, or /\n");
            return 1;
    }
    
    return 0;
}