module combinational_loop(a, y);
  input a;
  output y;
  wire w1, w2;
  and G1 (w1, a, w2);
  or  G2 (w2, w1, a);
  buf G3 (y, w2);
endmodule

