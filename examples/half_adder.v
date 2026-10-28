module half_adder(a, b, sum, carry);
  input a, b;
  output sum, carry;
  xor G1 (sum, a, b);
  and G2 (carry, a, b);
endmodule

