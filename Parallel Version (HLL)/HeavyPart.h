#include <valarray>
#include <cstring>
#include "BOBHash32.h"

class HeavyPart
{
private:
    static constexpr int hash_num = 3;
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
        depth = hash_num;
        width = mem * 1024 * 8 / depth / (hll_width*hll_bits*2+32*2+32);
        m = hll_width;
        alpha = 0.7213*m*m/(1+1.079/m);
        setmax = m*0.07;

        Superspreader = new bucket*[depth];
        for(int i=0; i<depth; i++){
            Superspreader[i] = new bucket[width];
            for(int j=0; j<width; j++){
                Superspreader[i][j].ca_v = new uint8_t[m]{};
                Superspreader[i][j].ca_nv = new uint8_t[m]{};
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
        uint32_t pos = ((hashval * ((uint64_t)m)) >> 32);
        uint8_t tmplevel = loghash(hashval);

        for (int i = 0; i < depth; i++) {
            uint32_t buc = ((hash[i]->run((const char *)&src, 4) * ((uint64_t)width)) >> 32);
            bucket *temp = &Superspreader[i][buc];
            if(temp->key == src){
                if(temp->ca_v[pos] < tmplevel)
                {
                    temp->vote += tmplevel - temp->ca_v[pos];
                    temp->ca_v[pos] = tmplevel;
                }
            }
            else if(temp->key == 0){
                temp->key = src;
                temp->vote = tmplevel;
                temp->ca_v[pos] = tmplevel;
            }
            else if(temp->ca_nv[pos] < tmplevel){
                uint32_t guard_val = temp->nvote + tmplevel -  temp->ca_nv[pos];
                if(guard_val/temp->vote >= lambda){
                    temp->key = src;
                    memset(temp->ca_v, 0, m * sizeof(uint8_t));
                    temp->ca_v[pos] = tmplevel;
                    temp->vote = tmplevel;

                    memset(temp->ca_nv, 0, m * sizeof(uint8_t));
                    temp->nvote = 0;
                }
                else{
                    temp->nvote += tmplevel -  temp->ca_nv[pos];
                    temp->ca_nv[pos] = tmplevel;
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
