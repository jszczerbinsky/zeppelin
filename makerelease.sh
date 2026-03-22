make_release() {
	rm *.o
	cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DEXE_NAME=$1 \
		-DTARGET_ARCH=$2 \
		-DTARGET_PLATFORM=$3 \
		-DEVAL_TYPE=$4 \
		-DPOPCNT=$5 \
		-DVECTORIZATION_METHOD=$6 \
		-DBIT_MANIPULATION=$7 \
		.
	cmake --build .
}

ver=$(git describe --tags --dirty --match v[0-9]*)
ver=$(echo "$ver" | sed -E 's/(-g[0-9a-f]+.*)$//')

rm -rf CMakeFiles/
rm CMakeCache.txt
rm ./build/*

make_release "zeppelin-${ver}-linux-x86_64-popcnt-avx2-bmi2"			"x86_64"  "Linux"   "EVAL_NNUE" "POPCNT"	"VECT_AVX2" "BIT_BMI2"
make_release "zeppelin-${ver}-linux-x86_64-popcnt-bmi2"					"x86_64"  "Linux"   "EVAL_NNUE" "POPCNT"	"VECT_NONE" "BIT_BMI2"
make_release "zeppelin-${ver}-linux-x86_64-popcnt-avx2"					"x86_64"  "Linux"   "EVAL_NNUE" "POPCNT"	"VECT_AVX2" "BIT_NONE"
make_release "zeppelin-${ver}-linux-x86_64-popcnt"						"x86_64"  "Linux"   "EVAL_NNUE" "POPCNT"	"VECT_NONE" "BIT_NONE"
make_release "zeppelin-${ver}-linux-x86_64-avx2-bmi2"					"x86_64"  "Linux"   "EVAL_NNUE" "NO_POPCNT" "VECT_AVX2" "BIT_BMI2"
make_release "zeppelin-${ver}-linux-x86_64-bmi2"						"x86_64"  "Linux"   "EVAL_NNUE" "NO_POPCNT" "VECT_NONE" "BIT_BMI2"
make_release "zeppelin-${ver}-linux-x86_64-avx2"						"x86_64"  "Linux"   "EVAL_NNUE" "NO_POPCNT" "VECT_AVX2" "BIT_NONE"
make_release "zeppelin-${ver}-linux-x86_64"								"x86_64"  "Linux"   "EVAL_NNUE" "NO_POPCNT" "VECT_NONE" "BIT_NONE"

make_release "zeppelin-${ver}-windows-x86_64-popcnt-avx2-bmi2.exe"		"x86_64"  "Windows" "EVAL_NNUE" "POPCNT"	"VECT_AVX2" "BIT_BMI2"
make_release "zeppelin-${ver}-windows-x86_64.popcnt-bmi2-exe"			"x86_64"  "Windows" "EVAL_NNUE" "POPCNT"	"VECT_NONE" "BIT_BMI2"
make_release "zeppelin-${ver}-windows-x86_64-popcnt-avx2.exe"			"x86_64"  "Windows" "EVAL_NNUE" "POPCNT"	"VECT_AVX2" "BIT_NONE"
make_release "zeppelin-${ver}-windows-x86_64.popcnt-exe"				"x86_64"  "Windows" "EVAL_NNUE" "POPCNT"	"VECT_NONE" "BIT_NONE"
make_release "zeppelin-${ver}-windows-x86_64-avx2-bmi2.exe"				"x86_64"  "Windows" "EVAL_NNUE" "NO_POPCNT" "VECT_AVX2" "BIT_BMI2"
make_release "zeppelin-${ver}-windows-x86_64-bmi2.exe"					"x86_64"  "Windows" "EVAL_NNUE" "NO_POPCNT" "VECT_NONE" "BIT_BMI2"
make_release "zeppelin-${ver}-windows-x86_64-avx2.exe"					"x86_64"  "Windows" "EVAL_NNUE" "NO_POPCNT" "VECT_AVX2" "BIT_NONE"
make_release "zeppelin-${ver}-windows-x86_64.exe"						"x86_64"  "Windows" "EVAL_NNUE" "NO_POPCNT" "VECT_NONE" "BIT_NONE"

make_release "zeppelin-${ver}-linux-aarch64-neon"						"aarch64" "Linux"   "EVAL_NNUE" "NO_POPCNT" "VECT_NEON" "BIT_NONE"
make_release "zeppelin-${ver}-linux-aarch64"							"aarch64" "Linux"   "EVAL_NNUE" "NO_POPCNT" "VECT_NONE" "BIT_NONE"
