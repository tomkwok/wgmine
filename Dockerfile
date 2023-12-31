# Builder image
FROM alpine AS builder
WORKDIR /build
RUN \
  apk add --no-cache \
    build-base \
    cmake

ARG ENABLE_O64=ON
COPY src ./src
RUN cmake src -DENABLE_O64=$ENABLE_O64 && make

# Project Image
FROM alpine
WORKDIR /
USER 1000
COPY --from=builder /build/wgmine .
ENTRYPOINT ["/wgmine"]