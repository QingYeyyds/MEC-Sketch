#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

#define MAX_INSERT_PACKAGE 10000000

class InputAdaptor{
    public:
        unordered_map<uint32_t, unordered_set<uint32_t>> ground_truth{};
        vector<pair<uint32_t, uint32_t>> insert_data{};
        int threshold{};

        InputAdaptor(const char* filename) {
            FILE *pf = fopen(filename, "rb");
            if (!pf) {
                cerr << filename << " not found." << endl;
                exit(-1);
            }

            char src[4], dst[4];
            pair<uint32_t,uint32_t> temp{};
            int ret = 0;
            while (true) {
                size_t rsize;
                rsize = fread(src, 1, 4, pf);
                if(rsize != 4) break;
                rsize = fread(dst, 1, 4, pf);
                if(rsize != 4) break;
                temp.first = *(uint32_t *) src;
                temp.second = *(uint32_t *) dst;
                insert_data.push_back(temp);
                sum.insert(temp);
                ground_truth[temp.first].insert(temp.second);
                ret++;
                if (ret == MAX_INSERT_PACKAGE){
                    cout << "MAX_INSERT_PACKAGE" << endl;
                    break;
                }
            }
            fclose(pf);

            threshold = 200;

            cout << "[Message] Read " << ret << " items" << endl;
            cout << "[Message] Total distinct paris: " << sum.size() << "  threshold=" << threshold<< endl;
        }

    private:
        struct edge_hash {
            size_t operator()(const pair<uint32_t, uint32_t>& p) const {
                return hash<uint32_t>()(p.first) ^ hash<uint32_t>()(p.second);
            }
        };

        unordered_set<pair<uint32_t,uint32_t>,edge_hash> sum{};
};
