uvec2 next;

void srand(uvec2 seed)
{
    next = seed;
}

vec2 hash(unsigned int R, unsigned int L, unsigned int k)
{
    unsigned int Rprime, Lprime;
    const unsigned int M = 0xCD9E8D57;
    int i;

    for (i = 0 ; i < 3 ; i += 1) {
        umulExtended(R, M, Rprime, Lprime);
        
        R = Rprime ^ L ^ k;
        L = Lprime;

        k += 0x9E3779B9;
    }

    next = uvec2(R, L);
    return vec2(R, L) / 4294967295.0 ;
}

vec2 rand()
{
    const uint a = 1664525u;
    const uint c = 1013904223u;
    next = next * a + c;
    return vec2 (next) / vec2(4294967295.0);
}
