#include <valarray>
#include "BOBHash32.h"

class HeavyPart
{
private:
    static constexpr int hash_num = 3;
    static constexpr int lambda = 8;

    static constexpr int mrb_bits = 128*4;
    static constexpr int mrb_b = 64;
    static constexpr int mrb_c = 8;

    int depth{};
    int width{};
    int m{};
    int segment{};
    int b{};
    int c{};
    int lastb{};
    int *offsets{};
    int setmax{};

    struct bucket{
        uint32_t key{};
        uint8_t* ca_v{};
        uint32_t vote{};
        uint8_t* ca_nv{};
        uint32_t nvote{};
    }** Superspreader;

    BOBHash32** hash;

    static uint8_t loghash(uint32_t p) {
        uint8_t ret = 0;
        while ((p&0x00000001) == 1) {
            p >>= 1;
            ret++;
        }
        return ret;
    }

#define getbit(n,bmp) ((bmp[(n)>>3])&(0x80>>((n)&0x07)))

#define setbit(n,bmp) {bmp[(n)>>3]|=(0x80>>((n)&0x07));}

#define fillzero(bmp) {for(int internali=0;internali<segment;internali++){bmp[internali]=0x00;}}

    static uint32_t countzerobits(const unsigned char* bmp, uint32_t from, uint32_t to){
        uint32_t z=0;
        for(uint32_t j=from;j<to;j++){
            if(!getbit(j,bmp)){
                z++;
            }
        }
        return z;
    }

    uint32_t estimate(unsigned char* bmp) const{
        uint32_t z, factor = 1;
        int base;
        for(base=0; base<c-1; base++){
            z = countzerobits(bmp,offsets[base],offsets[base]+b);
            if(b-z <= setmax){
                break;
            }
            else{
                factor = factor*2;
            }
        }

        double temp=0;
        int pos = base-1;
        for(int i=base; i<c-1; i++){
            z = countzerobits(bmp, offsets[i], offsets[i]+b);
            temp += 1.0 * b * log(1.0*b/z);
            if (z == 0 || b-z > setmax) {
                pos = i;
                temp = 0;
            }
        }
        factor = factor * (1 << (pos-base+1));

        z = countzerobits(bmp, offsets[c-1], offsets[c-1]+lastb);
        if(z == 0){
            temp += 1.0 * lastb * log(1.0*lastb);
        } else {
            temp += 1.0 * lastb * log(1.0*lastb/z);
        }
        return (uint32_t)(factor*temp + 0.5);
    }

public:
    HeavyPart(int mem){
        depth = hash_num;
        width = mem * 1024 * 8 / depth / (mrb_bits*2+32*2+32);
        m = mrb_bits;
        b = mrb_b;
        c = mrb_c;
        lastb = m-(c-1)*b;

        if (log(b)+log(2)*c>=32*log(2)) {
            cout << "Multiresolition bitmap too fine for 32 bit hash: b=" << b << "  c=" << c << std::endl;
            exit(-1);
        }
        if(log(b)+log(2)*c>27*log(2)){
            cout << "Unsafe DiscounterMRB for a 32 bit hash: b="<< b << "  c=" << c << std::endl;
            exit(-1);
        }

        double bitsetratio = 0.93105;
        setmax = int(b*bitsetratio + 0.5);

        offsets = new int[c+1]();
        for(int i=0; i<c; i++)
            offsets[i] = i*b;
        offsets[c] = offsets[c-1]+lastb;

        segment = (offsets[c] + 7) >> 3;

        Superspreader = new bucket*[depth];
        for(int i=0; i<depth; i++){
            Superspreader[i] = new bucket[width];
            for(int j=0; j<width; j++){
                Superspreader[i][j].ca_v = new uint8_t[segment]{};
                Superspreader[i][j].ca_nv = new uint8_t[segment]{};
            }
        }

        hash = new BOBHash32*[depth + 1];
        for (int i = 0; i < depth + 1; i++)
            hash[i] = new BOBHash32(1000+i);
    }

    void insert(uint32_t src, uint32_t dst)
    {
        uint64_t edge = src;
        edge = (edge << 32) | dst;
        uint32_t hashval = hash[depth]->run((const char *)&edge, 8);
        uint8_t tmplevel = loghash(hashval);
        uint32_t pos;
        if(tmplevel < c-1){
            pos = ((hashval * (uint64_t)b) >> 32);
            pos += offsets[tmplevel];
        }
        else {
            pos = ((hashval * ((uint64_t)lastb)) >> 32);
            pos += offsets[c-1];
        }

        for (int i = 0; i < depth; i++) {
            uint32_t buc = ((hash[i]->run((const char *)&src, 4) * ((uint64_t)width)) >> 32);
            bucket *temp = &Superspreader[i][buc];
            if(temp->key == src){
                if(getbit(pos, temp->ca_v)==0)
                {
                    setbit(pos, temp->ca_v)
                    temp->vote += 1;
                }
            }
            else if(temp->key == 0){
                temp->key = src;
                setbit(pos, temp->ca_v)
                temp->vote = 1;
            }
            else if(getbit(pos, temp->ca_nv)==0){
                uint32_t guard_val = temp->nvote + 1;
                if(guard_val/temp->vote >= lambda){
                    temp->key = src;
                    fillzero(temp->ca_v)
                    setbit(pos, temp->ca_v)
                    temp->vote = 1;

                    fillzero(temp->ca_nv)
                    temp->nvote = 0;
                }
                else{
                    setbit(pos, temp->ca_nv)
                    temp->nvote += 1;
                }
            }
        }
    }

    void query_superspreader(int thresh, vector<pair<uint32_t,uint32_t>> &results) {
        unordered_set<uint32_t> keyset{};
        for (int i = 0; i < depth; i++) {
            for (int j = 0; j < width; j++) {
                uint32_t est = estimate(Superspreader[i][j].ca_v);
                if (est >= thresh) {
                    keyset.insert(Superspreader[i][j].key);
                }
            }
        }

        for (uint32_t it: keyset) {
            pair<uint32_t, uint32_t> node{};
            node.first = it;
            node.second = 0;
            for (int i = 0; i < depth; i++) {
                uint32_t buc = ((hash[i]->run((const char *)&it, 4) * ((uint64_t)width)) >> 32);
                if(Superspreader[i][buc].key == it)
                    node.second = max(node.second, estimate(Superspreader[i][buc].ca_v));
            }
            results.push_back(node);
        }
    }

};
