double arith_seq_sum(double first, double step, double count,
                     unsigned long long d) {
  double last = first + count * step;
  return count * (first + last) * d;
}
