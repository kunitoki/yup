git clone https://github.com/kunitoki/yup.git
cd yup
cmake -S . -B build -DYUP_BUILD_TESTS=ON -DYUP_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel 4
