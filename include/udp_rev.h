#ifndef MY_APPLICATION_FFMPEG_PLAYER_UDP_RECEIVER_H
#define MY_APPLICATION_FFMPEG_PLAYER_UDP_RECEIVER_H

typedef enum
{
    SPS_FRAME = 1,
    PPS_FRAME = 2,
    E_FRAME = 3,
    D_START_PACKET = 4,
    I_FRAME = 6,
    D_REST_PACKET = 5,
    FRAME_DATA_ERROR = 7

} frame_type_e;

class UDP_Receiver
{

public:
    int sockfd;
    bool ifStartRender = false;

    void init_rev();
    frame_type_e get_frame_type(char *data, int packetLen);
    void insert_data_into_players_packet_queue(char *data, int dataLen);

private:

    bool if_frame_started(char *data);
    pthread_t revPid;
    // void insert_data_into_players_packet_queue_old(char *data, int dataLen);
};

#endif // MY_APPLICATION_FFMPEG_PLAYER_UDP_RECEIVER_H