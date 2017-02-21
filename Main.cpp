/*
 * Main.cpp
 *
 *  Created on: 2016年11月30日
 *      Author: louisia
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
using namespace std;

int runClients(char *argv[]) {
	struct sockaddr_in6 saddr, maddr;
	int sock_recv;
	struct ipv6_mreq mreq;
	char buf[1400];

	int on = 1, hops = 16, ifidx = 0;

	sock_recv = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_recv < 0) {
		perror("socket");
		return 1;
	}

	if (setsockopt(sock_recv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		perror("setsockopt");
		return 1;
	}

	if (setsockopt(sock_recv, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifidx,
			sizeof(ifidx))) {
		perror("setsockopt");
		return 1;
	}

	if (setsockopt(sock_recv, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops,
			sizeof(hops))) {
		perror("setsockopt");
		return 1;
	}

	if (setsockopt(sock_recv, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &on,
			sizeof(on))) {
		perror("setsockopt");
		return 1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin6_family = AF_INET6;
	saddr.sin6_port = htons(atoi(argv[3]));
	saddr.sin6_addr = in6addr_any;

	if (bind(sock_recv, (struct sockaddr *) &saddr, sizeof(saddr))) {
		perror("bind");
		return 1;
	}

	memset(&maddr, 0, sizeof(maddr));
	inet_pton(AF_INET6, argv[2], &maddr.sin6_addr);

	memcpy(&mreq.ipv6mr_multiaddr, &maddr.sin6_addr,
			sizeof(mreq.ipv6mr_multiaddr));
	mreq.ipv6mr_interface = ifidx;

	if (setsockopt(sock_recv, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *) &mreq,
			sizeof(mreq))) {
		perror("setsockopt");
		return 1;
	}
	int cnt = 0;
	while (1) {
		socklen_t addr_len = sizeof(saddr);
		memset(buf, 0, sizeof(buf));

		// 接收数据
		int n = recvfrom(sock_recv, buf, sizeof(buf), 0,
				(struct sockaddr*) &saddr, &addr_len);
		if (n == -1) {
			perror("recvfrom()");
		}

		printf("Recv %dst message from server:%s\n", cnt, buf);
		cnt++;

		//离开组播组，注意离开的时候，接收缓冲区中可能还有数据
		if (cnt == 5) {
			cout<<"leave"<<endl;
			setsockopt(sock_recv, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
					(char *) &mreq, sizeof(mreq));
		}
		sleep(2);
	}

	close(sock_recv);
	return 0;

}
int runServer(char *argv[]) {
	struct sockaddr_in6 multicast_addr;
	int sockfd;
	struct ipv6_mreq mreq;
	char buf[1400];
	int recv_len;
	int on = 1, hops = 16, ifindex = 0;

	sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0) {
		perror("socket");
		return 1;
	}
	//允许重用本地地址
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		perror("setsockopt");
		return 1;
	}
	//指定外出端口
	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex,
			sizeof(ifindex))) {
		perror("setsockopt");
		return 1;
	}
	//设置外出跳限
	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops,
			sizeof(hops))) {
		perror("setsockopt");
		return 1;
	}
	//指定允许回馈
	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &on,
			sizeof(on))) {
		perror("setsockopt");
		return 1;
	}

	memset(&multicast_addr, 0, sizeof(struct sockaddr_in6));
	multicast_addr.sin6_family = AF_INET6;
	multicast_addr.sin6_port = htons(atoi(argv[3]));
	inet_pton(AF_INET6, argv[2], &multicast_addr.sin6_addr);

	memcpy(&mreq.ipv6mr_multiaddr, &multicast_addr.sin6_addr,
			sizeof(mreq.ipv6mr_multiaddr));
	mreq.ipv6mr_interface = ifindex;

	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *) &mreq,
			sizeof(mreq))) {
		perror("setsockopt");
		return 1;
	}

	strcpy(buf, "hello");
	while (1) {
		recv_len = sendto(sockfd, buf, strlen(buf), 0,
				(struct sockaddr*) &multicast_addr, sizeof(multicast_addr));
		if (recv_len < 0) {
			perror("sendto()");
			return -2;
		}

		sleep(1);
	}

	close(sockfd);

	return 0;
}
int main(int argc, char *argv[]) {

	if (argc < 4) {
		printf(
				"\nUsage: %s <role> <address> <port>\n\nExample: %s client ff15::101 12345\n\n",
				argv[0], argv[0]);
		return 1;
	}
	if (strcmp(argv[1], "client") == 0) {
		cout << "runclient" << endl;
		runClients(argv);
	}
	if (strcmp(argv[1], "server") == 0) {
		cout << "runserver" << endl;
		runServer(argv);
	}
	return 0;
}
