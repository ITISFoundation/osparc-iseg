NI:=2:
NJ:=2:
M:=Matrix([ [m[1],m[2]], [m[3],m[4]] ]);
C([ det = Determinant(M) ]);
M1:=MatrixInverse(M);
# output in row-wise order
for j from 1 to NJ do
for i from 1 to NI do
#print(M1[j,i]);
C([ m1[(j-1)*NI+i] = applyrule(Determinant(M)=det,M1[j,i]) ]);
end do;
end do;



NI:=3:
NJ:=3:
M:=Matrix([ [m[1],m[2],m[3]], [m[4],m[5],m[6]], [m[7],m[8],m[9]] ]);
C([ det = Determinant(M) ]);
M1:=MatrixInverse(M);
# output in row-wise order
for j from 1 to NJ do
for i from 1 to NI do
#print(applyrule(Determinant(M)=det,M1[j,i]));
C([ m1[(j-1)*NI+i] = applyrule(Determinant(M)=det,M1[j,i]) ]);
end do;
end do;
