FROM centos:8

ADD ./ /tmp/linearizable_clock/

RUN yum install libevent-devel -y && yum group install "Development Tools" -y && \
	gcc --version && \
	cd /tmp/linearizable_clock && make

CMD cd /tmp/linearizable_clock && ./clockTest2 ${IP}
