#include "data.h"
#include "HeavyPart.h"

int main(){
    const char* filename = "caida_ip.dat";
    auto* adaptor =  new InputAdaptor(filename);

    int mem = 100;
    auto *sketch = new HeavyPart(mem);

    timespec dtime1{}, dtime2{};
    long long delay;
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for(const auto& it:adaptor->insert_data){
        sketch->insert(it.first,it.second);
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    printf("[Message] Update throughput: %lf Mops\n",(double)(1000)*(double)(adaptor->insert_data.size())/(double)delay);

    vector<pair<uint32_t, uint32_t>> results{};
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for(int i=0; i<10; i++){
        results.clear();
        sketch->query_superspreader(adaptor->threshold, results);
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    double delay_temp = double(dtime2.tv_sec - dtime1.tv_sec) * 1000 + (dtime2.tv_nsec - dtime1.tv_nsec) / 1e6;
    delay_temp /= 10;
    printf("[Message] Detection time: %.6f ms\n",delay_temp);

    double f1 = 0, precision = 0, recall = 0, aae = 0;
    int tp = 0, cnt = 0;
    for (const auto& it : adaptor->ground_truth) {
        int truth = (int)it.second.size();
        if(truth >= adaptor->threshold) {
            cnt++;
            for(auto &result : results) {
                if(result.first == it.first) {
                    tp++;
                    aae += abs((int)result.second - truth);
                    break;
                }
            }
        }
    }
    precision = tp*1.0/(int)results.size();
    recall = tp*1.0/cnt;
    f1 = 2*precision*recall/(precision+recall);
    aae = aae/tp;
    cout << "[Message] Total " << cnt << " superspreaders, detect " << tp << endl;
    cout<<"[Message] F1: "<<f1<<"\t Precision: "<<precision<<"\t Recall: "<<recall<<"\t AAE: "<<aae;

    return 0;
}