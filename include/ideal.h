
#ifndef IDEAL_H
#define IDEAL_H

#include <pari/pari.h>
#include <stdbool.h>

GEN alg_scalar(GEN A, GEN i);

GEN alg_conj(GEN A, GEN x);

// returns the gram matrix of the norm quadratic form with respect to algbasis(A)
GEN alg_gram(GEN A);

// returns a random integer linear combination of the basis of 'lattice'
// with coefficients in (-n,n)
GEN lattice_random(GEN A, GEN lattice, GEN n);

// returns an element of 'lattice' of prime norm
// generated as a random linear combination of basis elements with
// coefficients in (-n,n)
GEN lattice_random_prime(GEN A, GEN lattice, GEN n);

// same but l must be a quadratic residue modulo the prime norm
GEN lideal_equiv_prime_not_quadratic_residue(GEN I, GEN* alpha, GEN l) ;

// returns an element of 'lattice' of norm N such that gcd(a,N) = b
GEN lattice_random_gcd(GEN A, GEN lattice, GEN n, GEN a, GEN b);

// GEN left ideal has 3 components:
// 1) lattice
// 2) norm (0 if not computed yet)
// 3) x such that the ideal is generated by norm and x (0 if not computed yet)
// 4) the parent algebra
// 5) the parent order

// creates the left ideal in 'order' generated by the element 'x' and the integer 'N', divided by its content
GEN lideal_create(GEN A, GEN order, GEN x, GEN N);

// does not divide by the content of (x,N)
GEN lideal_create_safe(GEN A, GEN order, GEN x, GEN N);


// WARNING: the following may modify the input ideal (if a value was not precomputed)

GEN lideal_lattice(GEN lideal);

GEN alg_primitive(GEN *n, GEN A, GEN order, GEN x);

// We call a cyclic ideal an ideal corresponding to a cyclic isogeny (i.e., not divisible by an integer)
bool lideal_cyclic(GEN *lideal);

long remove_1_i(GEN A, GEN order, GEN *x);

long lideal_simplify(GEN *lideal);

GEN lideal_algebra(GEN lideal);

GEN lideal_order(GEN lideal);

GEN lideal_norm(GEN lideal);

GEN lideal_generator(GEN lideal);

// finds a generator of 'lideal' of norm coprime with M (assumes gcd(M, norm(lideal)) = 1)
GEN lideal_generator_coprime(GEN lideal, GEN M);

GEN lideal_mul(GEN I, GEN alpha); // I*alpha where I is a left-ideal and alpha an element of the algebra

GEN lideal_add(GEN I1, GEN I2);

GEN lideal_inter(GEN I1, GEN I2);

GEN lideal_inter_sum(GEN I1, GEN I2);

int lideal_equals(GEN I1, GEN I2);


int lattice_is_lideal(GEN A, GEN O, GEN lat); // check if lattice is a left ideal
int lideal_sanity_check(GEN lideal); // check that lideal is a well-formed ideal

// lideal_basis * lideal_scalar is a basis of the ideal lattice (lideal_basis has integral coefficients)
GEN lideal_basis(GEN lideal);
GEN lideal_scalar(GEN lideal);

// returns the quadratic form of the left ideal equal to the norm form divided by
// the scalar alglat_get_scalar(lideal_lattice(lideal))
GEN lideal_gram(GEN lideal);

// returns an LLL-reduced basis of the left ideal divided by
// the scalar alglat_get_scalar(lideal_lattice(lideal))
GEN lideal_lll(GEN lideal);

// returns the n shortest vectors
GEN lideal_short(GEN lideal, GEN B, GEN n);

// if isom is given, it is set to an element alpha in I such that I*conj(alpha)/N(I) = output
GEN lideal_equiv_prime(GEN I, GEN* alpha);

// same but the norm should not be in the list 'list_primes'
GEN lideal_equiv_prime_except(GEN I, GEN* alpha, GEN list_primes);

GEN lideal_equiv_prime_random(GEN I, GEN* alpha, GEN bound_coeff);
// GEN lideal_equiv_prime_random2(GEN I, GEN* alpha, GEN bound_coeff);
// the output has norm a (large) prime and possibly cofactors from the factorisation matrix fm
// try_at_least is the minimum number of enumerated vectors, in an attempts to minimize the large prime
// try_at_least = 0 returns the first valid solution
GEN lideal_equiv_nearprime(GEN lideal, GEN fm, unsigned int try_at_least);

// returns alpha such that I1*alpha = I2, and NULL if not isomorphic
GEN lideal_isom(GEN I1, GEN I2);

// random lideal of norm 2^e
GEN lideal_random_2e(GEN A, GEN order, long e);

//same as above but for norm 3^e
GEN lideal_random_3e(GEN A, GEN order, long e);

GEN lideal_primary_decomposition(GEN I, GEN fact_norm);

GEN lideal_right_order(GEN I);

#endif