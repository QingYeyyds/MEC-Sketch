#include "data.h"
#include "mec_sketch.h"

int main(){
    const char* filename = "caida_ip.dat";
    auto* adaptor = new InputAdaptor(filename);

    int mem = 300;
    double ratio = 0.3;
    auto* sketch = new MEC_Sketch(int(mem*ratio), int(mem*(1-ratio)));

    timespec dtime1{}, dtime2{};
    long long delay{};
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for(const auto& it:adaptor->insert_data){
        sketch->insert(it.first,it.second);
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    delay = (long long)(dtime2.tv_sec - dtime1.tv_sec) * 1000000000LL + (dtime2.tv_nsec - dtime1.tv_nsec);
    printf("[Message] Update throughput: %lf Mops\n",(double)(1000)*(double)(adaptor->insert_data.size())/(double)delay);

    double aae = 0, are = 0;
    clock_gettime(CLOCK_MONOTONIC, &dtime1);
    for(int i=0; i<10; i++) {
        aae = 0, are = 0;
        for (const auto& it : adaptor->ground_truth) {
            int real_value = (int)it.second.size();
            int query_val = sketch->query(it.first);
            aae += abs(real_value - query_val);
            are += 1.0 * abs(real_value - query_val)/real_value;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &dtime2);
    double delay_temp = double(dtime2.tv_sec - dtime1.tv_sec) * 1000 + (dtime2.tv_nsec - dtime1.tv_nsec) / 1e6;
    delay_temp /= 10;
    printf("[Message] Detection time: %.6f ms\n",delay_temp);

    aae /= (int)adaptor->ground_truth.size();
    are /= (int)adaptor->ground_truth.size();
    cout<<"[Message] AAE: "<<aae<<"\t ARE: "<<are;

    return 0;
}
