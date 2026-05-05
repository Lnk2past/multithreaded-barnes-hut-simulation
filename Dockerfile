FROM lnk2past/turtleshell:latest AS builder

WORKDIR /bh

COPY src src
COPY Makefile Makefile
COPY CMakeLists.txt CMakeLists.txt
COPY conanfile.py conanfile.py

RUN make

FROM lnk2past/turtleshell:latest

WORKDIR /bh

COPY pyproject.toml pyproject.toml
COPY .python-version .python-version
COPY uv.lock uv.lock
COPY app app
COPY --from=builder /bh/lib /bh/lib

RUN uv sync

ENV PYTHONPATH=/bh/lib

EXPOSE 5006

ENTRYPOINT ["uv", "run", "panel", "serve", "app", "--allow-websocket-origin=*"]