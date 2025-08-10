#include "HeavyPart.h"
#include "LightPart.h"

class MEC_Sketch
{
    HeavyPart* heavy_part{};
    LightPart* light_part{};

public:
    MEC_Sketch(int heavy_mem, int light_mem){
        heavy_part = new HeavyPart(heavy_mem);
        light_part = new LightPart(light_mem, HeavyPart::get_len());
    }

    void insert(uint32_t src, uint32_t dst)
    {
        uint32_t hash_pos{};
        uint32_t swap_src{};
        uint8_t *swap_val{};
        int result = heavy_part->insert(src, dst, hash_pos, swap_src, &swap_val);

        switch(result)
        {
            case 0: {
                delete swap_val;
                return;
            }
            case 1:{
                light_part->insert(src, hash_pos, swap_src, swap_val, true);
                delete swap_val;
                return;
            }
            case 2:
            {
                light_part->insert(src, hash_pos, swap_src, swap_val, false);
                delete swap_val;
                return;
            }
            default: {
                printf("error return value !\n");
                delete swap_val;
                exit(1);
            }
        }
    }

    int query(uint32_t src)
    {
        uint8_t *swap_val{};
        bool flag = false;
        int heavy_result = (int)heavy_part->query(src,&swap_val, flag);
        if(heavy_result == 0 || flag)
        {
            int light_result = (int)light_part->query(src, swap_val, flag);
            return heavy_result + light_result;
        }
        return heavy_result;
    }

};
