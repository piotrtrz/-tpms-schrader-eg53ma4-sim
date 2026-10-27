FROM python:3.12.12-slim-bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git libusb-1.0-0-dev librtlsdr-dev pkg-config ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /work
COPY tools/requirements.txt tools/requirements.txt
RUN pip install --no-cache-dir --disable-pip-version-check -r tools/requirements.txt
COPY . .
RUN chmod +x tools/check_all tools/*.sh
CMD ["bash", "./tools/check_all"]
