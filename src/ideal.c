#define _unused(x) ((void)(x))
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pari/pari.h>
#include <assert.h>


#include "ideal.h"
#include "toolbox.h"

GEN alg_scalar(GEN A, GEN i) {
    pari_sp ltop = avma;
    long n = alg_get_degree(A), N = nf_get_degree(alg_get_abssplitting(A));
    GEN res = zerocol(N*n);
    gel(res,1) = i;
    return gerepilecopy(ltop, res);
}

GEN alg_conj(GEN A, GEN x) {
    pari_sp ltop = avma;
    return gerepileupto(ltop,algsub(A,alg_scalar(A,algtrace(A,x,0)),x));
}

// returns the gram matrix of the norm quadratic form with respect to algbasis(A)
GEN alg_gram(GEN A) {
    pari_sp ltop = avma;

    GEN b = algbasis(A);
    GEN G = matid(4);

    for (int i = 1; i <= 4; ++i) {
        for (int j = 1; j <= 4; ++j)
        {
            gel(gel(G,i),j) = gsub(gsub(algnorm(A, algadd(A,gel(b,i), gel(b,j)), 0), algnorm(A, gel(b,i), 0)), algnorm(A, gel(b,j), 0));
            gel(gel(G,i),j) = gmul(gel(gel(G,i),j), ghalf);
        }
    }

    return gerepilecopy(ltop,G);
}

// returns a random integer linear combination of the basis of 'lattice'
// with coefficients in (-n,n)
GEN lattice_random(GEN A, GEN lattice, GEN n) {
    pari_sp ltop = avma;
    GEN r[4];
    for (int i = 0; i < 4; ++i) {
        r[i] = randomi(n);
        if (random_bits(1)) r[i] = gneg(r[i]);
    }
    GEN x = alglatelement(A, lattice, mkcol4(r[0], r[1], r[2], r[3]));
    return gerepileupto(ltop,x);
}


// returns an element of 'lattice' of prime norm
// generated as a random linear combination of basis elements with
// coefficients in (-n,n)
GEN lattice_random_prime(GEN A, GEN lattice, GEN n) {
    pari_sp ltop = avma;
    GEN x;
    do {
        x = lattice_random(A, lattice, n);
    } while (!(ispseudoprime(algnorm(A, x, 0),0)));
    return gerepilecopy(ltop,x);
}

// returns an element of 'lattice' of norm N such that gcd(a,N) = b
GEN lattice_random_gcd(GEN A, GEN lattice, GEN n, GEN a, GEN b) {
    pari_sp ltop = avma;
    GEN x;
    do {
        x = lattice_random(A, lattice, n);

    } while (gcmp(b,gcdii(a,algnorm(A, x, 0))));
    return gerepilecopy(ltop,x);
}

GEN alg_primitive(GEN *n, GEN A, GEN order, GEN x) {
    pari_sp ltop = avma;
    GEN X, y;
    bool ok = alglatcontains(A, order, x, &X);
    assert(ok);
    _unused(ok);
    *n = content(X);
    y = alglatelement(A, order, gdiv(X,*n));
    gerepileall(ltop, 2, n, &y);
    return y;
}

// GEN left ideal has 3 components:
// 1) lattice
// 2) norm (0 if not computed yet)
// 3) x such that the ideal is generated by norm and x (0 if not computed yet)
// 4) the parent algebra
// 5) the parent order

// creates the left ideal in 'order' generated by the element 'x' and the integer 'N', divided by its content
GEN lideal_create(GEN A, GEN order, GEN x, GEN N) {
    pari_sp ltop = avma;
    GEN lideal = cgetg(6,t_VEC);
    if (N) {
        GEN n;

        GEN y = alg_primitive(&n, A, order, x);
        GEN g = ggcd(n, N);
        GEN h = ggcd(algnorm(A,y,0),gdiv(N,g));


        g = gen_1; // ONLY CREATE IDEALS THAT ARE NOT DIVISIBLE BY AN INTEGER

        gel(lideal,2) = gmul(h,gsqr(g));
        gel(lideal,3) = gmul(g,y);
    }
    else {
        gel(lideal,2) = algnorm(A,x,0);
        gel(lideal,3) = gcopy(x);
    }
    gel(lideal,1) = gen_0; // the lattice; don't compute it until used

    gel(lideal,4) = A;
    gel(lideal,5) = gcopy(order);

    lideal_lattice(lideal);

    return gerepilecopy(ltop,lideal);
}

// does not divide by the content of (x,N)
GEN lideal_create_safe(GEN A, GEN order, GEN x, GEN N) {
    pari_sp ltop = avma;
    GEN I1 = lideal_create(A,order,x,NULL);
    GEN I2 = lideal_create(A,order,mkcol4(N,gen_0,gen_0,gen_0),NULL);
    GEN I = lideal_add(I1,I2);

    return gerepilecopy(ltop,I);
}



bool lideal_cyclic(GEN *lideal) {
    pari_sp ltop = avma;

    GEN n;
    GEN prim_gen = alg_primitive(&n, lideal_algebra(*lideal), lideal_order(*lideal), lideal_generator(*lideal));

    GEN g = ggcd(gsqr(n), lideal_norm(*lideal));
    GEN N = gdiv(lideal_norm(*lideal), g);

    *lideal = lideal_create(lideal_algebra(*lideal), lideal_order(*lideal), prim_gen, N);

    bool was_cyclic = gcmp(g, gen_1) == 0;
    *lideal = gerepilecopy(ltop,*lideal);
    return was_cyclic;
}

long lideal_simplify(GEN *lideal) {
    GEN i = mkcol4s(1,1,0,0);
    GEN x = lideal_generator(*lideal), n, N = lideal_norm(*lideal), y;
    long ctr = 0;
    while(Mod2(N) == 0) {
        y = algmul(lideal_algebra(*lideal),x,i);
        y = alg_primitive(&n, lideal_algebra(*lideal), lideal_order(*lideal), y);
        if (gcmp(n,gen_1) == 0) break;
        N = gdiv(N,gen_2);
        x = y;
        ctr++;
    }

    *lideal = lideal_create(lideal_algebra(*lideal), lideal_order(*lideal), x, N);
    return ctr;
}

long remove_1_i(GEN A, GEN order, GEN *x) {
    GEN i = mkcol4s(1,1,0,0);
    GEN n, N = algnorm(A,*x,0), y;
    long ctr = 0;
    while(Mod2(N) == 0) {
        y = algmul(A,*x,i);
        y = alg_primitive(&n, A, order, y);
        if (gcmp(n,gen_1) == 0) break;
        N = gdiv(N,gen_2);
        *x = y;
        ctr++;
    }

    return ctr;
}

GEN lideal_lattice(GEN lideal) {
    if (!gequal0(gel(lideal,1))) {
        return gel(lideal,1);
    }
    else {
        pari_sp ltop = avma;
        GEN B = lideal_algebra(lideal);
        GEN O = lideal_order(lideal);
        GEN lat_x = alglatmul(B, O, lideal_generator(lideal));
        if (gel(lideal,2) != gen_0) { // norm is given
            GEN lat_N;
            lat_N = alglatmul(B, O, alg_scalar(B, gel(lideal,2)));
            gel(lideal,1) = alglatadd(B, lat_x, lat_N, NULL);
        }
        else { // norm is not specified, meaning it is a principal ideal
            gel(lideal,1) = lat_x;
        }
        gel(lideal,1) = gerepilecopy(ltop,gel(lideal,1));
        return gel(lideal,1);
    }
}

GEN lideal_algebra(GEN lideal) { return gel(lideal,4); }

GEN lideal_order(GEN lideal) { return gel(lideal,5); }

GEN lideal_norm(GEN lideal) {
    if (!gequal0(gel(lideal,2))) {
        return gel(lideal,2);
    }
    else {
        pari_sp ltop = avma;
        Z_issquareall(alglatindex(lideal_algebra(lideal), lideal_lattice(lideal), lideal_order(lideal)), &gel(lideal,2));
        assert(!gequal0(gel(lideal,2)));
        gel(lideal,2) = gerepilecopy(ltop,gel(lideal,2));
        return gel(lideal,2);
    }
}

int lattice_is_lideal(GEN A, GEN O, GEN lat) {
    GEN basis_O = gmul(alglat_get_primbasis(O), alglat_get_scalar(O));
    GEN basis_lat = gmul(alglat_get_primbasis(lat), alglat_get_scalar(lat));

    for (int i = 1; i <= 4; ++i) {
        for (int j = 1; j <= 4; ++j) {
            int test = alglatcontains(A, lat, algmul(A,gel(basis_O,i),gel(basis_lat,j)), NULL);
            if (!test)
                return 0;
        }
    }

   return 1;
}

int lideal_sanity_check(GEN lideal) {
    GEN O = lideal_order(lideal);
    GEN A = lideal_algebra(lideal);
    GEN lat = lideal_lattice(lideal);
    return lattice_is_lideal(A,O,lat);
}

GEN lideal_generator(GEN lideal) {
    if (!gequal0(gel(lideal,3))) {
        return gel(lideal,3);
    }
    else {
        pari_sp ltop = avma;
        GEN norm = lideal_norm(lideal);
        GEN lat = lideal_lattice(lideal);

        gel(lideal,3) = lattice_random_gcd(lideal_algebra(lideal),lat,stoi(1000),gsqr(norm),norm);

        gel(lideal,3) = gerepilecopy(ltop,gel(lideal,3));

        return gel(lideal,3);
    }
}

// finds a generator of 'lideal' of norm N*k where k is coprime with M
GEN lideal_generator_coprime(GEN lideal, GEN M) {
    pari_sp ltop = avma;
    GEN norm = lideal_norm(lideal);
    GEN lat = lideal_lattice(lideal);

    GEN res = lattice_random_gcd(lideal_algebra(lideal),lat,stoi(1000),gmul(gsqr(norm),M),norm);


    return gerepilecopy(ltop,res);

}

GEN lideal_mul(GEN I, GEN alpha) {
    pari_sp ltop = avma;
    GEN A = lideal_algebra(I);
    GEN norm = algnorm(A,alpha,0);
    GEN generator = algmul(A,lideal_generator_coprime(I,Q_remove_denom(norm,NULL)),alpha);
    norm = gmul(lideal_norm(I),norm);
    GEN J = lideal_create(A, lideal_order(I), generator, norm);
    return gerepilecopy(ltop,J);
}

GEN lideal_add(GEN I1, GEN I2) {
    pari_sp ltop = avma;

    GEN lideal = cgetg(6,t_VEC);

    gel(lideal,1) = alglatadd(lideal_algebra(I1), lideal_lattice(I1), lideal_lattice(I2), NULL);
    gel(lideal,2) = gen_0;
    gel(lideal,3) = gen_0;
    gel(lideal,4) = lideal_algebra(I1);
    gel(lideal,5) = lideal_order(I1);

    lideal_norm(lideal);
    lideal_generator(lideal);

    return gerepilecopy(ltop,lideal);
}

GEN lideal_inter(GEN I1, GEN I2) {
    pari_sp ltop = avma;

    GEN lideal = cgetg(6,t_VEC);

    gel(lideal,1) = alglatinter(lideal_algebra(I1), lideal_lattice(I1), lideal_lattice(I2), NULL);
    gel(lideal,2) = gen_0;
    gel(lideal,3) = gen_0;
    gel(lideal,4) = lideal_algebra(I1);
    gel(lideal,5) = lideal_order(I1);

    lideal_norm(lideal);
    lideal_generator(lideal);


    GEN res = gerepilecopy(ltop,lideal);

    return res;
}

GEN lideal_inter_sum(GEN I1, GEN I2) {
    pari_sp ltop = avma;

    GEN inter = cgetg(6,t_VEC);
    GEN sum = cgetg(6,t_VEC);

    gel(inter,1) = alglatinter(lideal_algebra(I1), lideal_lattice(I1), lideal_lattice(I2), &sum);
    gel(inter,2) = gen_0;
    gel(inter,3) = gen_0;
    gel(inter,4) = lideal_algebra(I1);
    gel(inter,5) = lideal_order(I1);

    gel(sum,2) = gen_0;
    gel(sum,3) = gen_0;
    gel(sum,4) = lideal_algebra(I1);
    gel(sum,5) = lideal_order(I1);

    lideal_norm(inter);
    lideal_generator(inter);


    lideal_norm(sum);
    lideal_generator(sum);

    GEN result = mkvec2(inter, sum);

    return gerepilecopy(ltop,result);
}

int lideal_equals(GEN I1, GEN I2) {
    GEN index;
    int subset = alglatsubset(lideal_algebra(I1), lideal_lattice(I1), lideal_lattice(I2), &index);
    return (subset) && (gcmp(index, gen_1) == 0);
}

GEN lideal_basis(GEN lideal) {
    return alglat_get_primbasis(lideal_lattice(lideal));
}

GEN lideal_scalar(GEN lideal) {
    return alglat_get_scalar(lideal_lattice(lideal));
}

// returns the quadratic form of the left ideal equal to the norm form divided by
// the scalar alglat_get_scalar(lideal_lattice(lideal))
GEN lideal_gram(GEN lideal) {
    pari_sp ltop = avma;
    GEN L = lideal_basis(lideal);
    return gerepilecopy(ltop,gmul(gtrans(L), gmul(alg_gram(lideal_algebra(lideal)),L)));
}

// returns an LLL-reduced basis of the left ideal divided by
// the scalar alglat_get_scalar(lideal_lattice(lideal))
GEN lideal_lll(GEN lideal) {
    pari_sp ltop = avma;
    GEN basis = lideal_basis(lideal);
    GEN kerim = lllgramkerim(lideal_gram(lideal));
    GEN red = gmul(basis,gel(kerim,2));
    return gerepilecopy(ltop,red);
}

// returns the n shortest vectors
GEN lideal_short(GEN lideal, GEN B, GEN n) {
    pari_sp ltop = avma;
    GEN lattice = lideal_lattice(lideal);

    // find short vectors
    GEN res = qfminim0(lideal_gram(lideal), B, n, 2, 10); // should precision be higher? maybe gsizebyte(B)?

    // rewrite solutions in the basis of the algebra
    long cardinality = itos(gel(res,1))/2;
    for (int i = 1; i <= cardinality; ++i) {
        gel(gel(res,3),i) = alglatelement(lideal_algebra(lideal), lattice, gel(gel(res,3),i));
    }

    return gerepilecopy(ltop,res);
}

// this implementation looks for a equivalent prime ideal of SMALL norm,
// and does not work if the smallest equivalent prime ideal is too big (typically,
// when I itself has a very small norm)
// to avoid this issue, use lideal_equiv_prime_random
// if isom is given, it is set to an element alpha in I such that I*conj(alpha)/N(I) = output
GEN lideal_equiv_prime_except(GEN I, GEN* alpha, GEN list_primes) {
    pari_sp ltop = avma;

    GEN A = lideal_algebra(I);
    GEN N = lideal_norm(I);
    GEN lll = lideal_lll(I);
    GEN b = algnorm(A, gel(lll,1),0);
    long n, found = 0;
    GEN enumerate,v,M,norm_v,v_alg,res;
    GEN gram = lideal_gram(I);
    GEN scalar_2 = gsqr(lideal_scalar(I));

    long valuation_2 = vali(N);
    GEN N_odd = shifti(N,-valuation_2);

    while (!found) {
        res = qfminim0(gram, b, NULL, 2, 10); // should precision be higher? maybe gsizebyte(b)?
        n = itos(gel(res,1))/2;
        enumerate = gel(res,3);

        for (int i = 1; i <= n; ++i) {
            v = gel(enumerate, i);
            norm_v = gmul(qfeval(gram, v),scalar_2);

            M = shifti(norm_v,-valuation_2);
            if (Mod2(M) == 0) continue;

            M = diviiexact(M,N_odd);

            if (ispseudoprime(M,0)) {
                if (!list_primes || (!RgV_isin(list_primes, M))){
                        found = 1; break;
                    }
            }
        }
        b = gmulgs(b,2);
    }

    if (gcmp(N,M) == 0) {
        if (alpha) {
            *alpha = alg_scalar(A,gen_1);
            GEN res = gcopy(I);
            gerepileall(ltop, 2, alpha, &res);
            return res;
        }
        return gerepilecopy(ltop,I);
    }

    v_alg = alglatelement(A, lideal_lattice(I), v);

    GEN new_lideal = lideal_create(A, lideal_order(I), alg_conj(A,v_alg), M);


    if (alpha) {
        *alpha = v_alg;
        gerepileall(ltop, 2, alpha, &new_lideal);
        return new_lideal;
    }

    return gerepilecopy(ltop,new_lideal);
}

GEN lideal_equiv_prime(GEN I, GEN* alpha) {
    return lideal_equiv_prime_except(I, alpha, NULL);
}



GEN lideal_equiv_prime_random(GEN I, GEN* alpha, GEN bound_coeff) {
    pari_sp ltop = avma;

    GEN A = lideal_algebra(I);
    GEN N = lideal_norm(I);
    GEN lll = lideal_lll(I);
    GEN gram = gmul(gtrans(lll), gmul(alg_gram(A),lll));
    long found = 0, ctr = 0;
    GEN bound_ctr = gsqr(gsqr(bound_coeff));
    GEN scalar_2 = gsqr(lideal_scalar(I));

    long valuation_2 = vali(N);
    GEN N_odd = shifti(N,-valuation_2);

    GEN v,M,v_alg,norm_v;

    while (!found) {
        ctr++;
        v = mkcol4(randomi(bound_coeff), randomi(bound_coeff), randomi(bound_coeff), randomi(bound_coeff));
        norm_v = gmul(qfeval(gram, v),scalar_2);

        M = shifti(norm_v,-valuation_2);
        if (Mod2(M) == 0) continue;
        M = diviiexact(M,N_odd);

        if (ispseudoprime(M,0)) { v_alg = gmul(gmul(lll,v), lideal_scalar(I)); found = 1; break; }
        if (gcmp(stoi(ctr), bound_ctr) > 0) { // tried for too long, odds are no solution exists
            avma = ltop;
            return NULL;
        }
    }

    if (gcmp(N,M) == 0) {
        if (alpha) {
            *alpha = alg_scalar(A,gen_1);
            GEN res = gcopy(I);
            gerepileall(ltop, 2, alpha, &res);
            return res;
        }
        return gerepilecopy(ltop,I);
    }

    GEN new_lideal = lideal_create(A, lideal_order(I), alg_conj(A,v_alg), M);


    if (alpha) {
        *alpha = v_alg;
        gerepileall(ltop, 2, alpha, &new_lideal);
        return new_lideal;
    }

    return gerepilecopy(ltop,new_lideal);
}

//same as above but due to some bug we need it sometimes
// GEN lideal_equiv_prime_random2(GEN I, GEN* alpha, GEN bound_coeff) {
//     pari_sp ltop = avma;
//
//     GEN A = lideal_algebra(I);
//     GEN N = lideal_norm(I);
//     GEN lll = lideal_lll(I);
//     GEN gram = gmul(gtrans(lll), gmul(alg_gram(A),lll));
//     long found = 0, ctr = 0;
//     GEN bound_ctr = gsqr(gsqr(bound_coeff));
//     GEN scalar_2 = gsqr(lideal_scalar(I));
//
//     long valuation_2 = vali(N);
//     GEN N_odd = shifti(N,-valuation_2);
//
//     GEN v,M,v_alg,norm_v;
//
//     while (!found) {
//         ctr++;
//         v = mkcol4(randomi(bound_coeff), randomi(bound_coeff), randomi(bound_coeff), randomi(bound_coeff));
//         norm_v = gmul(qfeval(gram, v),scalar_2);
//
//         M = shifti(norm_v,-valuation_2);
//         if (Mod2(M) == 0) continue;
//         M = diviiexact(M,N_odd);
//
//         if (ispseudoprime(M,0)) { v_alg = gmul(gmul(lll,v), lideal_scalar(I)); found = 1; break; }
//         if (gcmp(stoi(ctr), bound_ctr) > 0) { // tried for too long, odds are no solution exists
//             avma = ltop;
//             return NULL;
//         }
//     }
//
//     if (gcmp(N,M) == 0) {
//         if (alpha) {
//           // printf("a \n");
//             // *alpha = alg_scalar(A,gen_1);
//             *alpha=v_alg;
//             GEN res = gcopy(I);
//             gerepileall(ltop, 2, alpha, &res);
//             return res;
//         }
//         return gerepilecopy(ltop,I);
//     }
//
//     GEN new_lideal = lideal_create(A, lideal_order(I), alg_conj(A,v_alg), M);
//
//
//     if (alpha) {
//       // printf("b \n");
//         *alpha = v_alg;
//         gerepileall(ltop, 2, alpha, &new_lideal);
//         return new_lideal;
//     }
//
//     return gerepilecopy(ltop,new_lideal);
// }


// the output has norm a (large) prime and possibly cofactors from the factorisation matrix fm
// try_at_least is the minimum number of enumerated vectors, in an attempts to minimize the large prime
// try_at_least = 0 returns the first valid solution

// TODO: for now, only odd primes allowed in fm
// also assumes that lideal is a cyclic integral ideal (not divisible by an integer)
GEN lideal_equiv_nearprime(GEN lideal, GEN fm, unsigned int try_at_least) {
    pari_sp ltop = avma;
    GEN A = lideal_algebra(lideal);
    GEN order = lideal_order(lideal);
    GEN N = lideal_norm(lideal);
    GEN lll = lideal_lll(lideal);
    GEN b = algnorm(A, gel(lll,1),0);
    unsigned int n = 0; //, found = 0;
    GEN enumerate,v,v_alg,M,M_reduced, res, norm_v;
    GEN gram = lideal_gram(lideal);
    GEN divisor;
    GEN scalar_2 = gsqr(lideal_scalar(lideal));

    long valuation_2 = vali(N), cyclic = 0;
    GEN N_odd = shifti(N,-valuation_2);


    GEN best_v_alg = NULL;
    GEN best_M = NULL, best_M_reduced = NULL;


    while (!best_v_alg || n < try_at_least) { // while none found, or found candidates but searching for better

        res = qfminim0(gram, b, NULL, 2, 10); // should precision be higher? maybe gsizebyte(b)?

                n = itos(gel(res,1))/2;
        enumerate = gel(res,3);

        for (unsigned int i = 1; i <= n; ++i) {
            v = gel(enumerate, i);
            norm_v = gmul(qfeval(gram, v),scalar_2);

            M = shifti(norm_v,-valuation_2);
            if (Mod2(M) == 0) continue;

            M = diviiexact(M,N_odd);

            // TODO: check M for small bad factors;
            // if (ugcd(2552041111277906183ULL, umodiu(M,2552041111277906183ULL)) != 1) continue;

            if ((!fm) || (lg(fm) == 1)) M_reduced = M; // famat_Z_gcd does not handle the empty matrix properly?
            else {
                divisor = famat_Z_gcd(fm, M);
                M_reduced = diviiexact(M, famat_prod(divisor));
            }

            if (ispseudoprime(M_reduced,0)) {
                // a candidate!
                if ((!best_v_alg) || gcmp(M_reduced,best_M_reduced) < 0) {
                    v_alg = alglatelement(A, lideal_lattice(lideal), v);

                    cyclic = 1;

                    // check that alpha has no scalar divisor in common with N
                    GEN z;
                    v_alg = alg_primitive(&z, A, order, v_alg);

                    if (gcmp(ggcd(z, N), gen_1) == 0) {
                        best_M_reduced = M_reduced;
                        best_M = M;
                        best_v_alg = v_alg;
                        if (i >= try_at_least) break; // stop searching (instead of trying to find a better one...)
                        if (n > try_at_least) n = try_at_least; // keep searching, but not too long
                    }
                }
            }
        }
        b = gmulgs(b,2);
    }


    GEN new_lideal = lideal_create(A, lideal_order(lideal), alg_conj(A,best_v_alg), best_M);

    return gerepilecopy(ltop,new_lideal);
}

// returns alpha such that I1*alpha = I2, and NULL if not isomorphic
GEN lideal_isom(GEN I1, GEN I2) {
    pari_sp ltop = avma;
    GEN transporter = alglatrighttransporter(lideal_algebra(I1), lideal_lattice(I1), lideal_lattice(I2));

    GEN L = alglat_get_primbasis(transporter);
    GEN a = alglat_get_scalar(transporter);

    // the quadratic form
    GEN qf = gmul(gtrans(L), gmul(alg_gram(lideal_algebra(I1)),L));

    // bound on the smallest vector in qf
    GEN B = gdiv(gdiv(lideal_norm(I2),lideal_norm(I1)),gsqr(a));
    GEN res = qfminim0(qf, B, stoi(1), 2, 10); // should precision be higher?

    // not isomorphic
    if (gcmp(gel(res,1),gen_0) == 0) { avma = ltop; return NULL; }

    // rewrite solution in the basis of the algebra
    GEN sol = alglatelement(lideal_algebra(I1), transporter, gel(gel(res,3),1));

    return gerepilecopy(ltop,sol);
}

// random lideal of norm 2^e
GEN lideal_random_2e(GEN A, GEN order, long e) {
    pari_sp ltop = avma;

    GEN I = lideal_create(A, order, alg_scalar(A, gen_1), gen_1); // trivial left ideal
    GEN O2 = alglatmul(A, order, alg_scalar(A, gen_2)); // trivial left ideal

    GEN x, a;

    for (int i = 0; i < e; ++i) {

        do {
            x = lattice_random(A, lideal_lattice(I), stoi(1000));
            a = gdiv(algnorm(A, x, 0), powgi(stoi(2),stoi(i))); // N(x)/N(I)
        } while ((umodiu(a, 2) != 0) || alglatcontains(A, O2, x, NULL)); // norm 2 step, non-backtraking

        I = lideal_create(A, order, x, powgi(stoi(2),stoi(i+1)));
    }

    return gerepilecopy(ltop,I);
}

// GEN lideal_random_3e(GEN A, GEN order, long e) {
//     pari_sp ltop = avma;
//     GEN l=ge
//     GEN I = lideal_create(A, order, alg_scalar(A, gen_1), gen_1); // trivial left ideal
//     GEN O2 = alglatmul(A, order, alg_scalar(A, gen_2)); // trivial left ideal
//
//     GEN x, a;
//
//     for (int i = 0; i < e; ++i) {
//
//         do {
//             x = lattice_random(A, lideal_lattice(I), stoi(1000));
//             a = gdiv(algnorm(A, x, 0), powgi(stoi(2),stoi(i))); // N(x)/N(I)
//         } while ((umodiu(a, 2) != 0) || alglatcontains(A, O2, x, NULL)); // norm 2 step, non-backtraking
//
//         I = lideal_create(A, order, x, powgi(stoi(2),stoi(i+1)));
//     }
//
//     return gerepilecopy(ltop,I);
// }
//random lideal of norm 3^e
GEN lideal_random_3e(GEN A, GEN order, long e) {
    pari_sp ltop = avma;
    GEN l=gadd(gen_1,gen_2);
    GEN I = lideal_create(A, order, alg_scalar(A, gen_1), gen_1); // trivial left ideal
    GEN Ol = alglatmul(A, order, alg_scalar(A,l)); // trivial left ideal

    GEN x, a;

    for (int i = 0; i < e; ++i) {


        do {
            x = lattice_random(A, lideal_lattice(I), stoi(1000));
            a = gdiv(algnorm(A, x, 0), powgi(stoi(3),stoi(i))); // N(x)/N(I)
        } while ((umodiu(a, 3) != 0) || alglatcontains(A, Ol, x, NULL)); // norm 2 step, non-backtraking

        I = lideal_create(A, order, x, powgi(stoi(3),stoi(i+1)));
    }
    return gerepilecopy(ltop,I);

}

GEN lideal_primary_decomposition(GEN I, GEN fact_norm) {
    pari_sp ltop = avma;

    GEN fact, gelle;
    long len = lg(gel(fact_norm,1));

    fact = cgetg(len, t_VEC);

    for (int i = 1; i < len; ++i) {
        gelle = powii(gel(gel(fact_norm,1),i),gel(gel(fact_norm,2),i));

        gel(fact,i) = lideal_create(lideal_algebra(I), lideal_order(I), lideal_generator(I), gelle);
    }

    return gerepilecopy(ltop,fact);
}


GEN lideal_right_order(GEN I) {
  pari_sp ltop = avma;

  GEN order = alglatrighttransporter(lideal_algebra(I), lideal_lattice(I), lideal_lattice(I));
  GEN res = gerepilecopy(ltop,order);

  return res;
}
