#include <valarray>
#include <cstring>
#include "BOBHash32.h"

class HeavyPart
{
private:
    static constexpr int bucket_num = 8;
    static constexpr int lambda = 8;

    static constexpr int hll_bits = 4;
    static constexpr int hll_width = 128;
    static constexpr int thre = (1<<hll_bits)-1;

    int depth{};
    int width{};
    int m{};
    double alpha{};
    double setmax{};

    struct bucket{
        uint32_t key{};
        uint8_t* ca_v{};
        uint32_t vote{};
    }** Superspreader;

    BOBHash32** hash{};

    static uint8_t loghash(uint32_t p) {
        uint8_t ret = 0;
        while ((p&0x00000001) == 1) {
            p >>= 1;
            ret++;
        }
        return min(ret+1,thre);
    }

    uint32_t estimate(const uint8_t* ca_v) const{
        double res = 0;
        int zeros = 0;
        for (int i = 0; i < m; i++) {
            res += 1.0/int((uint64_t)1 << ca_v[i]);
            zeros = ca_v[i] == 0 ? zeros+1 : zeros;
        }
        if (zeros > setmax) return uint32_t(m*log(m*1.0/zeros));
        return (uint32_t)(alpha/res);
    }

public:
    HeavyPart(int mem){
        depth = mem * 1024 * 8 / ((hll_width*hll_bits+32+32) * bucket_num);
        width = bucket_num;
        m = hll_width;
        alpha = 0.7213*m*m/(1+1.079/m);
        setmax = m*0.07;

        Superspreader = new bucket*[depth];
        for(int i=0; i<depth; i++){
            Superspreader[i] = new bucket[width];
            for(int j=0; j<width; j++){
                Superspreader[i][j].ca_v = new uint8_t[m]{};
            }
        }

        hash = new BOBHash32*[2];
        for (int i = 0; i < 2; i++)
            hash[i] = new BOBHash32(1000+i);
    }

    void insert(uint32_t src, uint32_t dst)
    {
        uint64_t edge = src;
        edge = (edge << 32) | dst;
        uint32_t hashval = hash[0]->run((const char *)&edge, 8);
        uint32_t pos = ((hashval * ((uint64_t)m)) >> 32);
        uint8_t tmplevel = loghash(hashval);

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
            if(temp[matched].ca_v[pos] < tmplevel)
            {
                temp[matched].vote += tmplevel - temp[matched].ca_v[pos];
                temp[matched].ca_v[pos] = tmplevel;
            }
            return;
        }

        if(empty != -1){
            temp[empty].key = src;
            temp[empty].ca_v[pos] = tmplevel;
            temp[empty].vote= tmplevel;
            return;
        }

        if(temp[width-1].ca_v[pos] < tmplevel)
        {
            uint32_t guard_val = temp[width-1].vote + tmplevel -  temp[width-1].ca_v[pos];
            if(guard_val/min_val >= lambda){
                temp[min_c].key = src;
                memset(temp[min_c].ca_v, 0, m * sizeof(uint8_t));
                temp[min_c].ca_v[pos] = tmplevel;
                temp[min_c].vote = tmplevel;

                memset(temp[width-1].ca_v, 0, m * sizeof(uint8_t));
                temp[width-1].vote = 0;
                return;
            }
            else{
                temp[width-1].vote += tmplevel -  temp[width-1].ca_v[pos];
                temp[width-1].ca_v[pos] = tmplevel;
                return;
            }
        }

    }

    void query_superspreader(int thresh, vector<pair<uint32_t,uint32_t>> &results) {
        for (int i = 0; i < depth; i++) {
            for (int j = 0; j < width-1; j++) {
                uint32_t est = estimate(Superspreader[i][j].ca_v);
                if (est >= thresh) {
                    results.emplace_back(Superspreader[i][j].key,est);
                }
            }
        }
    }

};
