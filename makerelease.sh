make_release() {
	rm *.o
	cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DEXE_NAME=$1 \
		-DTARGET_ARCH=$2 \
		-DTARGET_PLATFORM=$3 \
		-DEVAL_TYPE=$4 \
		-DVECTORIZATION_METHOD=$5 \
		.
	cmake --build .
}

ver=$(git describe --tags --dirty)

rm -rf CMakeFiles/
rm CMakeCache.txt
rm ./build/*

make_release "zeppelin-${ver}-linux-x86_64-avx2"       "x86_64"  "Linux"   "EVAL_NNUE" "VECT_AVX2"
make_release "zeppelin-${ver}-linux-x86_64"            "x86_64"  "Linux"   "EVAL_NNUE" "VECT_NONE"

make_release "zeppelin-${ver}-windows-x86_64-avx2.exe" "x86_64"  "Windows" "EVAL_NNUE" "VECT_AVX2"
make_release "zeppelin-${ver}-windows-x86_64.exe"      "x86_64"  "Windows" "EVAL_NNUE" "VECT_NONE"

make_release "zeppelin-${ver}-linux-aarch64-neon"      "aarch64" "Linux"   "EVAL_NNUE" "VECT_NEON"
make_release "zeppelin-${ver}-linux-aarch64"           "aarch64" "Linux"   "EVAL_NNUE" "VECT_NONE"
