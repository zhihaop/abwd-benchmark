# abwd-benchmark

Asynchronous Bulk Write Dispatcher (ABWD) is a asynchronous scheduler that receiveing user request, and batching them together asynchronously. The implementation is [here](https://github.com/taosdata/TDengine/pull/16866).

## Dependencies

```bash
sudo apt install -y build-essential cmake libspdlog-dev
```

## How to use

1. Install the taosc with ABWD feature.

    ```bash
    git clone https://github.com/zhihaop/TDengine/tree/improve/taosc-async-enhancement-for-2.6.git
    cd TDengine
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --parallel $(nproc)
    sudo make install
    ```

2. Start TDengine service.

    ```bash
    sudo systemctl start taosd
    ```

3. Build and run abwd-benchmark.

    ```bash
    git clone https://github.com/zhihaop/abwd-benchmark.git
    cd abwd-benchmark
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --parallel $(nproc)
    ./abwd-benchmark
    ```
