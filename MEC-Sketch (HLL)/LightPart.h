class LightPart
{
private:
    static constexpr int hash_num = 3;
    static constexpr int lc_bits = 16;
    static constexpr int capacity = int(lc_bits * 0.75);

    int depth{};
    int width{};
    int m{};
    int segment{};

    uint8_t*** lc{};

    int heavy_len{};
    int div{};

    BOBHash32** hash{};

#define getbit(n,bmp) ((bmp[(n)>>3])&(0x80>>((n)&0x07)))

#define setbit(n,bmp) {bmp[(n)>>3]|=(0x80>>((n)&0x07));}

#define orbit(bmp_dst,bmp_src) {for(int internali=0;internali<segment;internali++){bmp_dst[internali]|=bmp_src[internali];}}

#define andbit(bmp_dst,bmp_src) {for(int internali=0;internali<segment;internali++){bmp_dst[internali]&=bmp_src[internali];}}

    uint32_t countzerobits(const uint8_t* bmp) const{
        uint32_t z=0;
        for(int j=0;j<m;j++){
            if(!getbit(j,bmp)){
                z++;
            }
        }
        return z;
    }

    uint32_t estimate(uint8_t* bmp) const{
        uint32_t zeros = countzerobits(bmp);
        if(zeros == 0) zeros = 1;
        auto est = uint32_t(1.0 * m * log(1.0*m/zeros));
        return est;
    }

public:
    LightPart(int mem, int heavy_length){
        depth = hash_num;
        width = mem * 1024 * 8 / depth / lc_bits;
        m = lc_bits;
        segment = (m + 7) >> 3;

        lc = new uint8_t**[depth];
        for(int i=0; i<depth; i++){
            lc[i] = new uint8_t*[width];
            for(int j=0; j<width; j++){
                lc[i][j] = new uint8_t[segment]{};
            }
        }

        heavy_len = heavy_length;
        div = heavy_len/m;

        hash = new BOBHash32*[depth];
        for (int i = 0; i < depth; i++)
            hash[i] = new BOBHash32(750+i);
    }

    void insert(uint32_t src, uint32_t hash_pos, uint32_t swap_src, const uint8_t* val, bool flag)
    {
        if(flag){
            auto* temp = new uint8_t[segment]{};
            for(int i=0; i<m; i++){
                for(int j=i*div; j<(i+1)*div; j++){
                    if(val[j] != 0){
                        setbit(i,temp)
                        break;
                    }
                }
            }
            for (int i = 0; i < depth; i++) {
                uint32_t buc = ((hash[i]->run((const char *)&swap_src, 4) * ((uint64_t)width)) >> 32);
                orbit(lc[i][buc],temp)
            }
            delete[] temp;
        }
        else{
            uint32_t pos = hash_pos/div;
            for (int i = 0; i < depth; i++) {
                uint32_t buc = ((hash[i]->run((const char *) &src, 4) * ((uint64_t) width)) >> 32);
                setbit(pos,lc[i][buc])
            }
        }
    }

    uint32_t query(uint32_t src, const uint8_t* val, bool flag) {
        auto* temp = new uint8_t[segment]{};
        uint32_t buc = ((hash[0]->run((const char *)&src, 4) * ((uint64_t)width)) >> 32);
        memcpy(temp, lc[0][buc], segment * sizeof(uint8_t));
        for (int i = 1; i < depth; i++) {
            buc = ((hash[i]->run((const char *)&src, 4) * ((uint64_t)width)) >> 32);
            andbit(temp,lc[i][buc])
        }
        uint32_t query_val = 0;
        if(countzerobits(temp) >= m - capacity){
            query_val = estimate(temp);
        }
        else{
            query_val = uint32_t(1.0 * m * log(1.0*m/(m-capacity)));
        }

        if(flag){
            auto* temp_val = new uint8_t[segment]{};
            for(int i=0; i<m; i++){
                for(int j=i*div; j<(i+1)*div; j++){
                    if(val[j] != 0){
                        setbit(i,temp_val)
                        break;
                    }
                }
            }
            andbit(temp,temp_val)
            if(countzerobits(temp) >= m - capacity){
                query_val -= estimate(temp);
            }
            else{
                query_val -= uint32_t(1.0 * m * log(1.0*m/(m-capacity)));
            }
            delete[] temp_val;
        }

        delete[] temp;
        return query_val;
    }

};
