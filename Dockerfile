##################################
#       Streem Dockerfile.       #
# https://github.com/matz/streem #
##################################
FROM alpine

COPY . /usr/src/streem

RUN \
    apk update && \
    apk add \
      musl-dev gcc flex \
      bison gc-dev make \
    && \
    cd /usr/src/streem && \
    make && \
    apk del --purge \
        musl-dev gcc flex \
        bison gc-dev make \
    && \
    rm -rf /var/cache/apk/*

CMD /usr/src/streem/bin/streem
