#include<iostream>
#include<sstream>
#include<string>
#include<stdio.h>
#include<stdlib.h>
#include<vector>
#include<cstdlib>
#include<ctime>
#include<time.h>
#include<cmath>
#include"utils.h"
#include"udp_socket.h"
using namespace std;

#pragma pack(push)
#pragma pack(1)
//assume that byte order of device on bicycle is same with byte order of server 
struct Packet{
    uint32_t bicycle_id;
    uint32_t timestamp;
    double longtitude;
    double latitude;
    double angle;
    double velocity;
};
#pragma pack(pop)


const double kDpm = 8.9832*1e-6;//degree per meter
const double kBaseLongtitude = 116; // 116 E
const double kBaseLatitude = 40; //40 N
const double kBaseVelocity = 20; //20 km/h
const int kBaseId = 1000000;
const double kApd = 3.1415926 / 180; // arc per degree
const double kKmph2Mps = 1.0 / 3.6; // km/h to m/s

int Delay(int count=1000000){
    double x = 1;
    struct timeval pre, now;
    gettimeofday(&pre, NULL);
    //printf("delay start, time = %u s, %u us\n", now.tv_sec, now.tv_usec);
    for(int i = 0; i <= count; i ++){
        x *= 2;
        x += i;
    }
    for(int i = count; i >= 0; i --){
        x -= i;
        x /= 2;
    }
    gettimeofday(&now, NULL);
    //printf("delay end, time = %u s, %u us\n", now.tv_sec, now.tv_usec);
    return (now.tv_sec - pre.tv_sec)*1000000 + now.tv_usec - pre.tv_usec;
}


int main(int argc, char* argv[]){
    if(argc != 3){
        int delay_us = Delay(100);
        printf("delay us = %u us\n", delay_us);
        printf("usage : send dst_ip total_count\n");
        return -1;
    }
    char* dst_ip = argv[1];
    int total_count = atoi(argv[2]);
    printf("send to %s, total count = %d\n",dst_ip, total_count);
    UdpSocket send_sock;
    send_sock.Create();
    struct sockaddr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(50000);
    target_addr.sin_addr.s_addr = inet_addr(dst_ip);
    
    srand(time(0));

    vector<Packet> bicycles;
    struct timeval now;
    gettimeofday(&now, NULL);
    for(int i = 1; i <= 10000; i ++){
        bicycles.push_back(
                    {kBaseId + i, 
                    now.tv_sec, 
                    kBaseLongtitude + rand() % 100000*kDpm,
                    kBaseLatitude + rand() % 100000*kDpm,
                    rand() % 360 - 180,
                    kBaseVelocity + rand()%10 - 5
                    }); 
    } 
    struct timeval last_time;
    gettimeofday(&last_time, NULL);
    int count = 0;
    printf("start time  = %d s, %d us\n", last_time.tv_sec, last_time.tv_usec); 
    ostringstream ss;
    while(count < total_count){
        for(int i = 0; i < 10000; i ++){
            time_t tm_seconds = time((time_t*)NULL);
            bicycles[i].timestamp = tm_seconds;

            bicycles[i].longtitude += kDpm * kKmph2Mps * bicycles[i].velocity * 10 * cos(bicycles[i].angle *kApd); //every 10 seconds
            bicycles[i].latitude += kDpm * kKmph2Mps * bicycles[i].velocity * 10 * sin(bicycles[i].angle * kApd);
            bicycles[i].angle += (rand() % 10 - 5);
            bicycles[i].velocity  = kBaseVelocity + rand() % 10 - 5;
            ss.str("");
            ss << bicycles[i].bicycle_id << "@" <<  bicycles[i].timestamp << 
                "," << bicycles[i].longtitude << "," << bicycles[i].latitude << 
                "," << bicycles[i].angle << "," << bicycles[i].velocity;

            send_sock.SendTo(const_cast<char*>(ss.str().c_str()), ss.str().length(), target_addr);
            printf("send to server: %s\n", ss.str().c_str());
            int delay_us = Delay(100);
            count ++;
            if(count == total_count)
              break;
        }
    }
    gettimeofday(&last_time, NULL);
    printf("total send packet = %d, end time  = %d s, %d us\n", count, last_time.tv_sec, last_time.tv_usec); 
    return 0;
}
