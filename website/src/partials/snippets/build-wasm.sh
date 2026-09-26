git clone https://github.com/kunitoki/yup.git
cd yup
emcmake cmake -G "Ninja Multi-Config" -S . -B build -DYUP_BUILD_TESTS=ON -DYUP_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel 4
python3 -m http.server -d build
