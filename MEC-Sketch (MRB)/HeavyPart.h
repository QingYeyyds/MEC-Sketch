#include <valarray>
#include <cstring>
#include "BOBHash32.h"

class HeavyPart
{
private:
    static constexpr int bucket_num = 8;
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
        bool flag{};
    }** Superspreader;

    BOBHash32** hash{};

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
        depth = mem * 1024 * 8 / ((mrb_bits+32+32+1) * bucket_num);
        width = bucket_num;
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
            }
        }

        hash = new BOBHash32*[2];
        for (int i = 0; i < 2; i++)
            hash[i] = new BOBHash32(1000+i);
    }

    static int get_len(){
        return mrb_b;
    }

    int insert(uint32_t src, uint32_t dst, uint32_t& hash_pos, uint32_t& swap_src, uint8_t** swap_value)
    {
        uint64_t edge = src;
        edge = (edge << 32) | dst;
        uint32_t hashval = hash[0]->run((const char *)&edge, 8);
        uint8_t tmplevel = loghash(hashval);
        uint32_t pos;
        if(tmplevel < c-1){
            hash_pos = pos = ((hashval * (uint64_t)b) >> 32);
            pos += offsets[tmplevel];
        }
        else {
            hash_pos = pos = ((hashval * ((uint64_t)lastb)) >> 32);
            pos += offsets[c-1];
        }

        uint32_t buc = ((hash[1]->run((const char *)&src, 4) * ((uint64_t)depth)) >> 32);
        bucket *temp = Superspreader[buc];

        uint32_t min_val = UINT32_MAX;
        int matched = -1, empty = -1, min_c = -1;
        for (int i = 0; i < width-1; i++) {
            if(temp[i].key == src){
                matched = i;
                break;
            }

            if(temp[i].key == 0 && empty == -1){
                empty = i;
            }

            if(temp[i].vote < min_val){
                min_val = temp[i].vote;
                min_c = i;
            }
        }

        if(matched != -1){
            if(getbit(pos, temp[matched].ca_v)==0)
            {
                setbit(pos, temp[matched].ca_v)
                temp[matched].vote += 1;
            }
            return 0;
        }

        if(empty != -1){
            temp[empty].key = src;
            setbit(pos, temp[empty].ca_v)
            temp[empty].vote= 1;
            temp[empty].flag = false;
            return 0;
        }

        if(getbit(pos, temp[width-1].ca_v)==0) {
            uint32_t guard_val = temp[width-1].vote + 1;
            if (guard_val / min_val >= lambda) {
                swap_src = temp[min_c].key;
                int l = (mrb_b+7)>>3;
                *swap_value = new uint8_t[l]{};
                for(int i=0; i<l; i++){
                    for(int j=0; j<c; j++){
                        (*swap_value)[i] |= temp[min_c].ca_v[i+j*l];
                    }
                }

                temp[min_c].key = src;
                fillzero(temp[min_c].ca_v)
                setbit(pos, temp[min_c].ca_v)
                temp[min_c].vote = 1;
                temp[min_c].flag = true;

                fillzero(temp[width - 1].ca_v)
                temp[width - 1].vote = 0;
                return 1;
            }
            else{
                setbit(pos, temp[width - 1].ca_v)
                temp[width - 1].vote += 1;
                return 2;
            }
        }

        return 2;
    }

    uint32_t query(uint32_t src, uint8_t** swap_value, bool& flag) {
        uint32_t buc = ((hash[1]->run((const char *)&src, 4) * ((uint64_t)depth)) >> 32);
        for (int i = 0; i < width-1; i++){
            if(Superspreader[buc][i].key == src){
                if(Superspreader[buc][i].flag){
                    int l = (mrb_b+7)>>3;
                    *swap_value = new uint8_t[l]{};
                    for(int k=0; k<l; k++){
                        for(int j=0; j<c; j++){
                            (*swap_value)[k] |= Superspreader[buc][i].ca_v[k+j*l];
                        }
                    }
                    flag= true;
                }
                return estimate(Superspreader[buc][i].ca_v);
            }
        }
        return 0;
    }

};
