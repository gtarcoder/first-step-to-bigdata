#include<iostream>
#include<thread>
#include<tuple>
#include<string>
#include<sstream>
#include<stdio.h>
#include<stdlib.h>
#include<fstream>
#include<csignal>
#include<unistd.h>
#include<time.h>
#include"utils.h"
#include"fifo_queue.h"
#include"udp_socket.h"
#include<librdkafka/rdkafkacpp.h>
using namespace std;

bool run = true;
static void metadata_print (const std::string &topic,
                            const RdKafka::Metadata *metadata) {
  std::cout << "Metadata for " << (topic.empty() ? "" : "all topics")
           << "(from broker "  << metadata->orig_broker_id()
           << ":" << metadata->orig_broker_name() << std::endl;

  /* Iterate brokers */
  std::cout << " " << metadata->brokers()->size() << " brokers:" << std::endl;
  RdKafka::Metadata::BrokerMetadataIterator ib;
  for (ib = metadata->brokers()->begin();
       ib != metadata->brokers()->end();
       ++ib) {
    std::cout << "  broker " << (*ib)->id() << " at "
              << (*ib)->host() << ":" << (*ib)->port() << std::endl;
  }
  /* Iterate topics */
  std::cout << metadata->topics()->size() << " topics:" << std::endl;
  RdKafka::Metadata::TopicMetadataIterator it;
  for (it = metadata->topics()->begin();
       it != metadata->topics()->end();
       ++it) {
    std::cout << "  topic \""<< (*it)->topic() << "\" with "
              << (*it)->partitions()->size() << " partitions:";

    if ((*it)->err() != RdKafka::ERR_NO_ERROR) {
      std::cout << " " << err2str((*it)->err());
      if ((*it)->err() == RdKafka::ERR_LEADER_NOT_AVAILABLE)
        std::cout << " (try again)";
    }
    std::cout << std::endl;

    /* Iterate topic's partitions */
    RdKafka::TopicMetadata::PartitionMetadataIterator ip;
    for (ip = (*it)->partitions()->begin();
         ip != (*it)->partitions()->end();
         ++ip) {
      std::cout << "    partition " << (*ip)->id()
                << ", leader " << (*ip)->leader()
                << ", replicas: ";

      /* Iterate partition's replicas */
      RdKafka::PartitionMetadata::ReplicasIterator ir;
      for (ir = (*ip)->replicas()->begin();
           ir != (*ip)->replicas()->end();
           ++ir) {
        std::cout << (ir == (*ip)->replicas()->begin() ? "":",") << *ir;
      }

      /* Iterate partition's ISRs */
      std::cout << ", isrs: ";
      RdKafka::PartitionMetadata::ISRSIterator iis;
      for (iis = (*ip)->isrs()->begin(); iis != (*ip)->isrs()->end() ; ++iis)
        std::cout << (iis == (*ip)->isrs()->begin() ? "":",") << *iis;

      if ((*ip)->err() != RdKafka::ERR_NO_ERROR)
        std::cout << ", " << RdKafka::err2str((*ip)->err()) << std::endl;
      else
        std::cout << std::endl;
    }
  }
}



class ExampleDeliveryReportCb : public RdKafka::DeliveryReportCb {
 public:
  void dr_cb (RdKafka::Message &message) {

    std::cout << "Message delivery for (" << message.len() << " bytes): " <<
        message.errstr() << std::endl;
    if (message.key())
      std::cout << "Key: " << *(message.key()) << ";" << std::endl;
  }
};
class ExampleEventCb : public RdKafka::EventCb {
 public:
  void event_cb (RdKafka::Event &event) {
    switch (event.type())
    {
      case RdKafka::Event::EVENT_ERROR:
        std::cerr << "ERROR (" << RdKafka::err2str(event.err()) << "): " <<
            event.str() << std::endl;
        if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN)
          run = false;
        break;

      case RdKafka::Event::EVENT_STATS:
        std::cerr << "\"STATS\": " << event.str() << std::endl;
        break;

      case RdKafka::Event::EVENT_LOG:
        fprintf(stderr, "LOG-%i-%s: %s\n",
                event.severity(), event.fac().c_str(), event.str().c_str());
        break;

      default:
        std::cerr << "EVENT " << event.type() <<
            " (" << RdKafka::err2str(event.err()) << "): " <<
            event.str() << std::endl;
        break;
    }
  }
};

/* Use of this partitioner is pretty pointless since no key is provided
 * in the produce() call. */
class MyHashPartitionerCb : public RdKafka::PartitionerCb {
 public:
  int32_t partitioner_cb (const RdKafka::Topic *topic, const std::string *key,
                          int32_t partition_cnt, void *msg_opaque) {
    return djb_hash(key->c_str(), key->size()) % partition_cnt;
  }
 private:

  static inline unsigned int djb_hash (const char *str, size_t len) {
    unsigned int hash = 5381;
    for (size_t i = 0 ; i < len ; i++)
      hash = ((hash << 5) + hash) + str[i];
    return hash;
  }
};

static void sigterm (int sig) {
  run = false;
  exit(1);
}
static bool exit_eof = false;
std::string brokers = "localhost";
std::string errstr = "error!";
std::string topic_str;
std::string mode;
std::string debug;
int32_t partition = RdKafka::Topic::PARTITION_UA;
int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING;
bool do_conf_dump = false;
int opt;
MyHashPartitionerCb hash_partitioner;
int use_ccb = 0;

const int kMaxSize = 1000000;
int total_count = 0;


std::tuple<RdKafka::Producer*, RdKafka::Topic*> InitRdKafka(int argc, char* argv[]){
    if(argc != 2){
        std::cout << "usage: receive count" << std::endl;
        exit(1);
    }
    brokers = "kafka1:9092,kafka2:9092,kafka3:9092";
    topic_str = "bicycle_track";
    run = true;
    total_count = atoi(argv[1]);
    printf("total count to write = %d\n", total_count);

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);
    //init kafka
    /*
    * Create configuration objects
    */
    RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    RdKafka::Conf *tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    
    /*
    * Set configuration properties
    */
    conf->set("metadata.broker.list", brokers, errstr);

    ExampleEventCb ex_event_cb;
    conf->set("event_cb", &ex_event_cb, errstr);
    /*
     * Create producer using accumulated global configuration.
     */
    RdKafka::Producer *producer = RdKafka::Producer::create(conf, errstr);
    if (!producer) {
      std::cerr << "Failed to create producer: " << errstr << std::endl;
      exit(1);
    }

    std::cout << "% Created producer " << producer->name() << std::endl;

    /*
     * Create topic handle.
     */
    RdKafka::Topic *topic = RdKafka::Topic::create(producer, topic_str,
						   tconf, errstr);
    if (!topic) {
      std::cerr << "Failed to create topic: " << errstr << std::endl;
      exit(1);
    }
    return std::make_tuple(producer, topic);
}

void ProcsFunc(RdKafka::Producer* producer, RdKafka::Topic* topic){
    RdKafka::ErrorCode resp; 
    char procs_buffer[1000];
    int procs_len = 50;
    uint32_t procs_count = 0;
    time_t tm_seconds;


    time_t tm_start = time((time_t*)NULL);
    printf("start to procss, time = %u\n", tm_start);

    struct timeval cur_time;
    ostringstream ss;

    while(procs_count < total_count){
        procs_count ++;
        if (procs_count % 100000 == 0){
            tm_seconds = time((time_t*)NULL);
            printf("procsing, count = %u, time = %u\n", procs_count, tm_seconds);
            while(producer->outq_len()){
                producer->poll(10);
            }
        }

        ss.str("");
        gettimeofday(&cur_time, NULL);
        uint64_t timestamp = cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000;
        ss << "1007451@" << timestamp << ",116.078,40.0434,35,21";
        while(true){
            resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
                  const_cast<char*>(ss.str().c_str()), ss.str().length(),
                  NULL, NULL);
            //printf("pushing %s to kafka, resp = %d\n", procs_buffer, resp); 
            if (resp == RdKafka::ERR__QUEUE_FULL){
                producer->poll(1000);
                continue;
            }
            if(resp != RdKafka::ERR_NO_ERROR)
            {
                printf("producer send error %s\n", RdKafka::err2str(resp).c_str());
            }else
              break;
        }
    }
    if (procs_count % 100000 == 0){
        tm_seconds = time((time_t*)NULL);
        printf("procsing, count = %u, time = %u\n", procs_count, tm_seconds);
        while(producer->outq_len()){
            producer->poll(10);
        }
    }


    time_t tm_end = time((time_t*)NULL);
    printf("end to procss, time = %u\n", tm_end);
    printf("total use seconds = %d\n", tm_end - tm_start);
}


int main(int argc, char* argv[]){
    std::tuple<RdKafka::Producer*, RdKafka::Topic*> param = InitRdKafka(argc, argv); 
    ProcsFunc(std::get<0>(param), std::get<1>(param));
    return 0;
}
