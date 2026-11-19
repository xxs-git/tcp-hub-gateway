#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#define CAN_IF "vcan0"
#define MAX_EVENTS 10

//打印can帧内容
void printf_can_frame(struct can_frame *frame)
{
  printf("can_id:0x%X | 数据长度:%d | data: ",frame->can_id & CAN_EFF_MASK, frame->can_dlc);
  for(int i = 0; i < frame->can_dlc;i++)
  {
    printf("%02X", frame->data[i]);
  }
  printf("\n");
}

int main(int argc, const char *argv[])
{
	 
	int can_fd, epoll_fd;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct epoll_event ev, events[MAX_EVENTS];
	struct can_filter filter;

	can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(can_fd < 0)
	{
      perror("socket can");
	  return -1;
	}

	strcpy(ifr.ifr_name,CAN_IF);
	ioctl(can_fd,SIOCGIFINDEX,&ifr);
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
    bind(can_fd,(struct sockaddr *)&addr, sizeof(addr));

	filter.can_id = 0x123;
	filter.can_mask = CAN_SFF_MASK;

	setsockopt(can_fd, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));


	epoll_fd = epoll_create1(0);
	ev.data.fd = can_fd;
	ev.events = EPOLLIN;
	epoll_ctl(epoll_fd,EPOLL_CTL_ADD,can_fd,&ev);

	printf("epoll监听vcan0,只接受0x123帧\n");

	while(1)
	{
        int nfds = epoll_wait(epoll_fd,events, MAX_EVENTS, -1);
		for(int i=0;i < nfds; i++)
		{
			if(events[i].data.fd == can_fd)
			{
              struct can_frame recv_frame;
			  ssize_t ret = read(can_fd,&recv_frame, sizeof(recv_frame));
			  if(ret == sizeof(recv_frame))
			  {
				  printf_can_frame(&recv_frame);
			  }
			}
		}
	}
    close(can_fd);
	close(epoll_fd);


	return 0;
}
