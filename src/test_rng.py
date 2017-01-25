import rng_python

s = rng_python.soliton_distribution(10);
g = rng_python.soliton_generator();

print(s.K())
for i in range(0, 10):
    print(s.generate(g))
