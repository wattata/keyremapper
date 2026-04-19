BINARY = keyremapper
CMD    = ./cmd/keyremapper

.PHONY: build universal test vet clean install

build:
	go build -o $(BINARY) $(CMD)

universal:
	GOARCH=arm64 go build -o $(BINARY)-arm64 $(CMD)
	GOARCH=amd64 go build -o $(BINARY)-amd64 $(CMD)
	lipo -create -output $(BINARY) $(BINARY)-arm64 $(BINARY)-amd64
	rm $(BINARY)-arm64 $(BINARY)-amd64

test:
	go test ./...

vet:
	go vet ./...

clean:
	rm -f $(BINARY) $(BINARY)-arm64 $(BINARY)-amd64

install: build
	cd install && ./install.sh
