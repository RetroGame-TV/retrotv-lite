plugins { id("com.android.application"); id("org.jetbrains.kotlin.android") }
val testAbi = providers.gradleProperty("testAbi").orNull
// Local update builds can retain the exact installed core binary, keeping its save-state identity.
val packagedQuicknes = providers.gradleProperty("packagedQuicknes").orNull == "true"
if (packagedQuicknes) require(file("../.tools/preserved-cores/${testAbi ?: "armeabi-v7a"}/libquicknes_libretro.so").isFile)
if (testAbi != null) { require(testAbi == "x86"); layout.buildDirectory.set(file("build-emulator")) }
android {
    namespace = "org.retrotv.lite"
    compileSdk = 35
    ndkVersion = "26.3.11579264"
    defaultConfig {
        applicationId = "org.retrotv.lite"
        minSdk = 26
        targetSdk = 32
        versionCode = 17
        versionName = "0.7.4-dev"
        ndk { abiFilters += (testAbi ?: "armeabi-v7a") }
        externalNativeBuild { cmake {
            cppFlags += listOf("-std=c++17", "-Wall", "-Wextra"); arguments += "-DANDROID_STL=c++_shared"
            if (packagedQuicknes) arguments += "-DRETROTV_PACKAGED_QUICKNES=ON"
        } }
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }
    externalNativeBuild { cmake { path = file("../native/CMakeLists.txt"); version = "3.22.1" } }
    sourceSets["main"].java.srcDir("../third_party/SDL/android-project/app/src/main/java")
    sourceSets["main"].assets.srcDir("../assets")
    sourceSets["main"].jniLibs.srcDir("../cores/packaged")
    if (packagedQuicknes) sourceSets["main"].jniLibs.srcDir("../.tools/preserved-cores")
    if (providers.gradleProperty("privateCores").orNull == "true") sourceSets["main"].jniLibs.srcDir("../cores/private")
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
    kotlinOptions { jvmTarget = "17" }
    packaging { jniLibs.useLegacyPackaging = true }
    buildTypes { release { isMinifyEnabled = false } }
    lint { abortOnError = true; lintConfig = file("lint.xml"); disable += "ExpiredTargetSdkVersion" }
}
dependencies {
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test:runner:1.6.2")
    androidTestImplementation("androidx.test.ext:junit:1.2.1")
}
