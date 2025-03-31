#include <stdio.h>

struct ExprVector {};

int myFunction() {
    int myVariable = 42;
    return myVariable;
}

ExprVector ExprVector::From(hParam x, hParam y, hParam z) {
    ExprVector ve;
    ve.x = Expr::From(x);
    ve.y = Expr::From(y);
    ve.z = Expr::From(z);
    return ve;
}
