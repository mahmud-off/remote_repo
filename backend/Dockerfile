FROM golang:1.21-alpine AS builder
WORKDIR /app
COPY main.go go.mod ./
RUN CGO_ENABLED=0 GOOS=linux go build -o tapalka .

FROM alpine:latest
WORKDIR /root/
COPY --from=builder /app/tapalka .
EXPOSE 8080
CMD ["./tapalka"]
