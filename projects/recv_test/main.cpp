#include<iostream>
#include<thread>
#include<vector>
#include<tuple>
#include<string>
#include<stdio.h>
#include<stdlib.h>
#include<fstream>
#include<csignal>
#include<signal.h>
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

vector<FifoQueue*> fifo_queues;
UdpSocket recv_sock;
const int kMaxSize = 1000000;



std::tuple<RdKafka::Producer*, RdKafka::Topic*> InitRdKafka(){
    brokers = "kafka1:9092,kafka2:9092,kafka3:9092";
    topic_str = "bicycle_track";
    run = true;
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

      /* Set delivery report callback */
    //ExampleDeliveryReportCb ex_dr_cb;
    //conf->set("dr_cb", &ex_dr_cb, errstr);

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

int gRecvCount = 0;
void ReceiveFunc(){
    const int kMaxBufSize = 10000;
    char recv_buffer[kMaxBufSize];
    int recv_len;
    int recv_count = 0;
    int fifo_index = 0;
    while(run){
        recv_len = recv_sock.Recv(recv_buffer, kMaxBufSize);
        if(recv_len > 0){
            fifo_queues[fifo_index]->PushPacketMutex(recv_buffer, recv_len);

            fifo_index = (fifo_index + 1) % fifo_queues.size();
            //fifo_queue.PushPacketLockFree(recv_buffer, recv_len);
            recv_count ++;            
            gRecvCount = recv_count;
        }
    }
}

void ProcsFunc(RdKafka::Producer* producer, RdKafka::Topic* topic, FifoQueue* fifo_queue){
    const int kMaxBufSize = 5000;
    uint16_t procs_len;
    char procs_buffer[kMaxBufSize];
    RdKafka::ErrorCode resp; 
    uint32_t procs_count = 0;
    time_t tm_seconds;
    while(run){
        tm_seconds = time((time_t*)NULL);
        fifo_queue->PopPacketMutex(procs_buffer, &procs_len);

        procs_count ++;
        if (procs_count % 100000 == 0){
            printf("$$$$$$$$$$$$$process, count = %u, time = %u\n", procs_count, tm_seconds);
            while(producer->outq_len() ){
                producer->poll(10);
            }
        }
        //fifo_queue.PopPacketLockFree(procs_buffer, &procs_len);
        while(true){
            resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY /* Copy payload */,
                  procs_buffer, procs_len,
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
}

int main(int argc, char* argv[]){
    if(argc != 2){
        printf("usage : receive write_thread_num\n");
        return -1;
    }

    int s, sig;
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGINT);


    int write_thread_num = atoi(argv[1]);
    recv_sock.Create();
    recv_sock.SetReusePort(true);
    recv_sock.Bind("0.0.0.0", 10001);
    recv_sock.SetRecvBufSize(2*1024*1024);

	int recv_buf_size;
	socklen_t optlen = 4;
	getsockopt(recv_sock.socket_fd(), SOL_SOCKET, SO_RCVBUF, (char*)&recv_buf_size, &optlen);
	printf("recv buf size = %d\n", recv_buf_size);
    
    std::thread recv_thread = std::thread(ReceiveFunc); 
    vector<std::thread> procs_threads;
    for(int i = 0; i < write_thread_num; i ++){
        fifo_queues.push_back(new FifoQueue);
        std::tuple<RdKafka::Producer*, RdKafka::Topic*> param = InitRdKafka(); 
        procs_threads.push_back(std::thread(std::bind(ProcsFunc, std::get<0>(param), std::get<1>(param), fifo_queues.back())));
    }

    s = pthread_sigmask(SIG_SETMASK, &sset, NULL);
    if (s != 0)
        perror("pthread sig mask error!\n");

    s = sigwait(&sset, &sig);
    if (s != 0){
        perror("sigwait error!\n");
    }else{
        printf("total received records = %d\n", gRecvCount);
    }
    return 0;
}
