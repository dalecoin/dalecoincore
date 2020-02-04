FROM alpine:3.8

ENV RPC_USER dalecoinrpc
ENV RPC_PASS pegforever

ADD dalecoind /usr/bin/dalecoind
ADD dalcstart.sh /dalcstart.sh
ENTRYPOINT ["/dalcstart.sh"]
