module multiple_driver(a, b, y);
  input a, b;
  output y;
  and G1 (y, a, b);
  or  G2 (y, a, b);
endmodule

