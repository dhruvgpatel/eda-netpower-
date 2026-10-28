module full_adder(a, b, cin, sum, cout);
  input a, b, cin;
  output sum, cout;
  wire s1, c1, c2;
  xor G1 (s1, a, b);
  xor G2 (sum, s1, cin);
  and G3 (c1, a, b);
  and G4 (c2, s1, cin);
  or  G5 (cout, c1, c2);
endmodule

