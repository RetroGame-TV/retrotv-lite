plugins { id("com.android.application"); id("org.jetbrains.kotlin.android") }
val testAbi = providers.gradleProperty("testAbi").orNull
if (testAbi != null) { require(testAbi == "x86"); layout.buildDirectory.set(file("build-emulator")) }
android {
    namespace = "org.retrotv.lite"
    compileSdk = 35
    ndkVersion = "26.3.11579264"
    defaultConfig {
        applicationId = "org.retrotv.lite"
        minSdk = 23
        targetSdk = 32
        versionCode = 1
        versionName = "0.1.0-dev"
        ndk { abiFilters += (testAbi ?: "armeabi-v7a") }
        externalNativeBuild { cmake { cppFlags += listOf("-std=c++17", "-Wall", "-Wextra"); arguments += "-DANDROID_STL=c++_shared" } }
        testInstrumentationRunner = "android.test.InstrumentationTestRunner"
    }
    externalNativeBuild { cmake { path = file("../native/CMakeLists.txt"); version = "3.22.1" } }
    sourceSets["main"].java.srcDir("../third_party/SDL/android-project/app/src/main/java")
    sourceSets["main"].assets.srcDir("../assets")
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
    kotlinOptions { jvmTarget = "17" }
    packaging { jniLibs.useLegacyPackaging = true }
    buildTypes { release { isMinifyEnabled = false } }
    lint { abortOnError = true; disable += "ExpiredTargetSdkVersion" }
}
dependencies { testImplementation("junit:junit:4.13.2") }
