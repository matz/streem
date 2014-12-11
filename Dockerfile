FROM debian
RUN apt-get update && apt-get install --no-install-recommends -y build-essential flex bison
ADD src/ /usr/src/streem/
WORKDIR /usr/src/streem
RUN make
CMD /usr/src/streem/streem